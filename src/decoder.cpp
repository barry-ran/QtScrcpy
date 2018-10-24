#include <QDebug>

#include "decoder.h"
#include "frames.h"

#define BUFSIZE 0x10000

Decoder::Decoder()
{

}

Decoder::~Decoder()
{

}

bool Decoder::init()
{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif
    if (avformat_network_init()) {
        return false;
    }
    return true;
}

void Decoder::deInit()
{
    avformat_network_deinit(); // ignore failure
}

void Decoder::setFrames(Frames *frames)
{
    m_frames = frames;
}

static qint32 readPacket(void *opaque, quint8 *buf, qint32 bufSize) {
    Decoder *decoder = (Decoder*)opaque;
    if (decoder) {
        return decoder->recvData(buf, bufSize);
    }
    return 0;
}

void Decoder::setDeviceSocket(QTcpSocket* deviceSocket)
{
    m_deviceSocket = deviceSocket;
}

qint32 Decoder::recvData(quint8* buf, qint32 bufSize)
{
    if (!buf) {
        return 0;
    }
    if (m_deviceSocket) {
        while (!m_quit && m_deviceSocket->bytesAvailable() < bufSize) {
            if (!m_deviceSocket->waitForReadyRead(300)
                    && QTcpSocket::SocketTimeoutError != m_deviceSocket->error()) {
                break;
            }
            if (QTcpSocket::SocketTimeoutError == m_deviceSocket->error()) {
                //qDebug() << "QTcpSocket::SocketTimeoutError";
            }
        }
        qDebug() << "recv data " << bufSize;
        return m_deviceSocket->read((char*)buf, bufSize);
    }
    return 0;
}

bool Decoder::startDecode()
{
    if (!m_deviceSocket) {
        return false;
    }
    m_quit = false;
    start();
    return true;
}

void Decoder::stopDecode()
{
    m_quit = true;
    if (m_frames) {
        m_frames->stop();
    }
    wait();
}

void Decoder::run()
{
    unsigned char *decoderBuffer = Q_NULLPTR;
    AVIOContext *avioCtx = Q_NULLPTR;
    AVFormatContext *formatCtx = Q_NULLPTR;
    AVCodec *codec = Q_NULLPTR;
    AVCodecContext *codecCtx = Q_NULLPTR;
    bool isFormatCtxOpen = false;
    bool isCodecCtxOpen = false;

    // decoder buffer
    decoderBuffer = (unsigned char*)av_malloc(BUFSIZE);
    if (!decoderBuffer) {
        qCritical("Could not allocate buffer");
        goto runQuit;
    }

    // io context
    avioCtx = avio_alloc_context(decoderBuffer, BUFSIZE, 0, this, readPacket, NULL, NULL);
    if (!avioCtx) {
        qCritical("Could not allocate avio context");
        // avformat_open_input takes ownership of 'decoderBuffer'
        // so only free the buffer before avformat_open_input()
        av_free(decoderBuffer);
        goto runQuit;
    }

    // format context
    formatCtx = avformat_alloc_context();
    if (!formatCtx) {
        qCritical("Could not allocate format context");
        goto runQuit;
    }
    formatCtx->pb = avioCtx;
    if (avformat_open_input(&formatCtx, NULL, NULL, NULL) < 0) {
        qCritical("Could not open video stream");
        goto runQuit;
    }
    isFormatCtxOpen = true;

    // codec
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        qCritical("H.264 decoder not found");
        goto runQuit;
    }

    // codec context
    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        qCritical("Could not allocate decoder context");
        goto runQuit;
    }
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        qCritical("Could not open H.264 codec");
        goto runQuit;
    }
    isCodecCtxOpen = true;

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = Q_NULLPTR;
    packet.size = 0;

    while (!m_quit && !av_read_frame(formatCtx, &packet)) {
        AVFrame* decodingFrame = m_frames->decodingFrame();
        // the new decoding/encoding API has been introduced by:
        // <http://git.videolan.org/?p=ffmpeg.git;a=commitdiff;h=7fc329e2dd6226dfecaa4a1d7adf353bf2773726>
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 37, 0)
        int ret;
        if ((ret = avcodec_send_packet(codecCtx, &packet)) < 0) {
            qCritical("Could not send video packet: %d", ret);
            goto runQuit;
        }
        if (decodingFrame) {
            ret = avcodec_receive_frame(codecCtx, decodingFrame);
        }
        if (!ret) {            
            // a frame was received
            pushFrame();

            //emit getOneFrame(yuvDecoderFrame->data[0], yuvDecoderFrame->data[1], yuvDecoderFrame->data[2],
            //        yuvDecoderFrame->linesize[0], yuvDecoderFrame->linesize[1], yuvDecoderFrame->linesize[2]);

            /*
            // m_conver转换yuv为rgb是使用cpu转的，占用cpu太高，改用opengl渲染yuv
            // QImage的copy也非常占用内存，此方案不考虑
            if (!m_conver.isInit()) {
                qDebug() << "decoder frame format" << decodingFrame->format;
                m_conver.setSrcFrameInfo(codecCtx->width, codecCtx->height, AV_PIX_FMT_YUV420P);
                m_conver.setDstFrameInfo(codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB32);
                m_conver.init();
            }
            if (!outBuffer) {
                outBuffer=new quint8[avpicture_get_size(AV_PIX_FMT_RGB32, codecCtx->width, codecCtx->height)];
                avpicture_fill((AVPicture *)rgbDecoderFrame, outBuffer, AV_PIX_FMT_RGB32, codecCtx->width, codecCtx->height);
            }            
            m_conver.convert(decodingFrame, rgbDecoderFrame);
            //QImage tmpImg((uchar *)outBuffer, codecCtx->width, codecCtx->height, QImage::Format_RGB32);
            //QImage image = tmpImg.copy();
            //emit getOneImage(image);
            */
        } else if (ret != AVERROR(EAGAIN)) {
            qCritical("Could not receive video frame: %d", ret);
            av_packet_unref(&packet);
            goto runQuit;
        }
#else
        while (packet.size > 0) {
            int gotPicture = 0;
            int len = -1;
            if (decodingFrame) {
                len = avcodec_decode_video2(codecCtx, decodingFrame, &gotpicture, &packet);
            }
            if (len < 0) {
                qCritical("Could not decode video packet: %d", len);
                goto runQuit;
            }
            if (gotPicture) {
                pushFrame();
            }
            packet.size -= len;
            packet.data += len;
        }
#endif
        av_packet_unref(&packet);

        if (avioCtx->eof_reached) {
            break;
        }
    }
    qDebug() << "End of frames";

runQuit:
        if (avioCtx) {
            av_freep(&avioCtx);
        }
        if (formatCtx && isFormatCtxOpen) {
            avformat_close_input(&formatCtx);
        }
        if (formatCtx) {
            avformat_free_context(formatCtx);
        }
        if (codecCtx && isCodecCtxOpen) {
            avcodec_close(codecCtx);
        }
        if (codecCtx) {
            avcodec_free_context(&codecCtx);
        }

        if (m_deviceSocket) {
            m_deviceSocket->disconnectFromHost();
            delete m_deviceSocket;
        }
}

void Decoder::pushFrame()
{
    bool previousFrameConsumed = m_frames->offerDecodedFrame();
    if (!previousFrameConsumed) {
        // the previous newFrame will consume this frame
        return;
    }
    emit newFrame();
}
