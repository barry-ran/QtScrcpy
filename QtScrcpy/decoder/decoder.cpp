#include <QDebug>
#include <QTime>

#include "decoder.h"
#include "frames.h"
#include "devicesocket.h"
#include "recorder.h"

#define BUFSIZE 0x10000
#define HEADER_SIZE 12
#define NO_PTS UINT64_C(-1)

typedef qint32 (*ReadPacketFunc)(void*, quint8*, qint32);

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

static quint32 bufferRead32be(quint8* buf) {
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static quint64 bufferRead64be(quint8* buf) {
    quint32 msb = bufferRead32be(buf);
    quint32 lsb = bufferRead32be(&buf[4]);
    return ((quint64) msb << 32) | lsb;
}

static Decoder::FrameMeta* frameMetaNew(quint64 pts) {
    Decoder::FrameMeta* meta = new Decoder::FrameMeta;
    if (!meta) {
        return meta;
    }
    meta->pts = pts;
    meta->next = Q_NULLPTR;
    return meta;
}

static void frameMetaDelete(Decoder::FrameMeta* frameMeta) {
    if (frameMeta) {
        delete frameMeta;
    }
}

static bool receiverStatePushMeta(Decoder::ReceiverState* state, quint64 pts) {
    Decoder::FrameMeta* frameMeta = frameMetaNew(pts);
    if (!frameMeta) {
        return false;
    }

    // append to the list
    // (iterate to find the last item, in practice the list should be tiny)
    Decoder::FrameMeta **p = &state->frameMetaQueue;
    while (*p) {
        p = &(*p)->next;
    }
    *p = frameMeta;
    return true;
}

static quint64 receiverStateTakeMeta(Decoder::ReceiverState* state) {
    Decoder::FrameMeta *frameMeta = state->frameMetaQueue; // first item
    Q_ASSERT(frameMeta); // must not be empty
    quint64 pts = frameMeta->pts;
    state->frameMetaQueue = frameMeta->next; // remove the item
    frameMetaDelete(frameMeta);
    return pts;
}

static qint32 readPacketWithMeta(void *opaque, uint8_t *buf, int bufSize) {
    Decoder* decoder = (Decoder*)opaque;
    Decoder::ReceiverState* state = decoder->getReceiverState();

    // The video stream contains raw packets, without time information. When we
    // record, we retrieve the timestamps separately, from a "meta" header
    // added by the server before each raw packet.
    //
    // The "meta" header length is 12 bytes:
    // [. . . . . . . .|. . . .]. . . . . . . . . . . . . . . ...
    //  <-------------> <-----> <-----------------------------...
    //        PTS        packet        raw packet
    //                    size
    //
    // It is followed by <packet_size> bytes containing the packet/frame.

    if (!state->remaining) {
        quint8 header[HEADER_SIZE];
        qint32 r = decoder->recvData(header, HEADER_SIZE);
        if (r == -1) {
            return AVERROR(errno);
        }
        if (r == 0) {
            return AVERROR_EOF;
        }
        // no partial read (net_recv_all())
        if (r != HEADER_SIZE) {
            return AVERROR(ENOMEM);
        }

        uint64_t pts = bufferRead64be(header);
        state->remaining = bufferRead32be(&header[8]);

        if (pts != NO_PTS && !receiverStatePushMeta(state, pts)) {
            qCritical("Could not store PTS for recording");
            // we cannot save the PTS, the recording would be broken
            return AVERROR(ENOMEM);
        }
    }

    Q_ASSERT(state->remaining);

    if (bufSize > state->remaining) {
        bufSize = state->remaining;
    }

    qint32 r = decoder->recvData(buf, bufSize);
    if (r == -1) {
        return AVERROR(errno);
    }
    if (r == 0) {
        return AVERROR_EOF;
    }

    Q_ASSERT(state->remaining >= r);
    state->remaining -= r;
    return r;
}

static qint32 readRawPacket(void *opaque, quint8 *buf, qint32 bufSize) {
    Decoder *decoder = (Decoder*)opaque;
    if (decoder) {
        qint32 len = decoder->recvData(buf, bufSize);
        if (len == -1) {
            return AVERROR(errno);
        }
        if (len == 0) {
            return AVERROR_EOF;
        }
        return len;
    }
    return AVERROR_EOF;
}

void Decoder::setDeviceSocket(DeviceSocket* deviceSocket)
{
    m_deviceSocket = deviceSocket;
}

void Decoder::setRecoder(Recorder *recorder)
{
    m_recorder = recorder;
}

qint32 Decoder::recvData(quint8* buf, qint32 bufSize)
{
    if (!buf) {
        return 0;
    }
    if (m_deviceSocket) {        
        qint32 len = m_deviceSocket->subThreadRecvData(buf, bufSize);
        return len;
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

Decoder::ReceiverState *Decoder::getReceiverState()
{
    return &m_receiverState;
}

void Decoder::run()
{    
    unsigned char *decoderBuffer = Q_NULLPTR;
    AVIOContext *avioCtx = Q_NULLPTR;
    AVFormatContext *formatCtx = Q_NULLPTR;
    AVCodec *codec = Q_NULLPTR;
    AVCodecContext *codecCtx = Q_NULLPTR;
    ReadPacketFunc readPacket = Q_NULLPTR;
    bool isFormatCtxOpen = false;
    bool isCodecCtxOpen = false;    

    // decoder buffer
    decoderBuffer = (unsigned char*)av_malloc(BUFSIZE);
    if (!decoderBuffer) {
        qCritical("Could not allocate buffer");
        goto runQuit;
    }

    // initialize the receiver state
    m_receiverState.frameMetaQueue = Q_NULLPTR;
    m_receiverState.remaining = 0;

    // if recording is enabled, a "header" is sent between raw packets
    readPacket = m_recorder ? readPacketWithMeta: readRawPacket;

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

    if (m_recorder && !m_recorder->open(codec)) {
        qCritical("Could not open recorder");
        goto runQuit;
    }

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
            char errorbuf[255] = { 0 };
            av_strerror(ret, errorbuf, 254);
            qCritical("Could not send video packet: %s", errorbuf);
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
                av_packet_unref(&packet);
                goto runQuit;
            }
            if (gotPicture) {
                pushFrame();
            }
            packet.size -= len;
            packet.data += len;
        }
#endif
        if (m_recorder) {
            // we retrieve the PTS in order they were received, so they will
            // be assigned to the correct frame
            quint64 pts = receiverStateTakeMeta(&m_receiverState);
            packet.pts = pts;
            packet.dts = pts;
            // no need to rescale with av_packet_rescale_ts(), the timestamps
            // are in microseconds both in input and output
            if (!m_recorder->write(&packet)) {
                qCritical("Could not write frame to output file");
                av_packet_unref(&packet);
                goto runQuit;
            }
        }

        av_packet_unref(&packet);

        if (avioCtx->eof_reached) {
            break;
        }
    }
    qDebug() << "End of frames";  

runQuit:
    if (m_recorder) {
        m_recorder->close();
    }
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

    emit onDecodeStop();
}

void Decoder::pushFrame()
{
    bool previousFrameConsumed = m_frames->offerDecodedFrame();
    if (!previousFrameConsumed) {
        // the previous newFrame will consume this frame
        return;
    }    
    emit onNewFrame();
}
