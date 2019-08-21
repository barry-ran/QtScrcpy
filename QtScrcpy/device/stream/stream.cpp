#include <QDebug>
#include <QTime>

#include "compat.h"
#include "stream.h"
#include "decoder.h"
#include "videosocket.h"
#include "recorder.h"

#define BUFSIZE 0x10000
#define HEADER_SIZE 12
#define NO_PTS UINT64_C(-1)

typedef qint32 (*ReadPacketFunc)(void*, quint8*, qint32);

Stream::Stream(QObject *parent)
    : QThread(parent)
{
    m_quit = false;
}

Stream::~Stream()
{

}

static void avLogCallback(void *avcl, int level, const char *fmt, va_list vl) {
    Q_UNUSED(avcl);
    Q_UNUSED(vl);

    QString localFmt = QString::fromUtf8(fmt);
    localFmt.prepend("[FFmpeg] ");
    switch (level) {
    case AV_LOG_PANIC:
    case AV_LOG_FATAL:
        qFatal(localFmt.toUtf8());
        break;
    case AV_LOG_ERROR:
        qCritical(localFmt.toUtf8());
        break;
    case AV_LOG_WARNING:
        qWarning(localFmt.toUtf8());
        break;
    case AV_LOG_INFO:
        qInfo(localFmt.toUtf8());
        break;
    case AV_LOG_DEBUG:
        //qDebug(localFmt.toUtf8());
        break;
    }

    // do not forward others, which are too verbose
    return;
}

bool Stream::init()
{
#ifdef QTSCRCPY_LAVF_REQUIRES_REGISTER_ALL
    av_register_all();
#endif
    if (avformat_network_init()) {
        return false;
    }
    av_log_set_callback(avLogCallback);
    return true;
}

void Stream::deInit()
{
    avformat_network_deinit(); // ignore failure
}

void Stream::setDecoder(Decoder* decoder)
{
    m_decoder = decoder;
}

static quint32 bufferRead32be(quint8* buf) {
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static quint64 bufferRead64be(quint8* buf) {
    quint32 msb = bufferRead32be(buf);
    quint32 lsb = bufferRead32be(&buf[4]);
    return ((quint64) msb << 32) | lsb;
}

static Stream::FrameMeta* frameMetaNew(quint64 pts) {
    Stream::FrameMeta* meta = new Stream::FrameMeta;
    if (!meta) {
        return meta;
    }
    meta->pts = pts;
    meta->next = Q_NULLPTR;
    return meta;
}

static void frameMetaDelete(Stream::FrameMeta* frameMeta) {
    if (frameMeta) {
        delete frameMeta;
    }
}

static bool receiverStatePushMeta(Stream::ReceiverState* state, quint64 pts) {
    Stream::FrameMeta* frameMeta = frameMetaNew(pts);
    if (!frameMeta) {
        return false;
    }

    // append to the list
    // (iterate to find the last item, in practice the list should be tiny)
    Stream::FrameMeta **p = &state->frameMetaQueue;
    while (*p) {
        p = &(*p)->next;
    }
    *p = frameMeta;
    return true;
}

static quint64 receiverStateTakeMeta(Stream::ReceiverState* state) {
    Stream::FrameMeta *frameMeta = state->frameMetaQueue; // first item
    Q_ASSERT(frameMeta); // must not be empty
    quint64 pts = frameMeta->pts;
    state->frameMetaQueue = frameMeta->next; // remove the item
    frameMetaDelete(frameMeta);
    return pts;
}

static qint32 readPacketWithMeta(void *opaque, uint8_t *buf, int bufSize) {
    Stream* stream = (Stream*)opaque;
    Stream::ReceiverState* state = stream->getReceiverState();

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
        qint32 r = stream->recvData(header, HEADER_SIZE);
        if (r == -1) {
            return errno ? AVERROR(errno) : AVERROR_EOF;
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

    qint32 r = stream->recvData(buf, bufSize);
    if (r == -1) {
        return errno ? AVERROR(errno) : AVERROR_EOF;
    }
    if (r == 0) {
        return AVERROR_EOF;
    }

    Q_ASSERT(state->remaining >= r);
    state->remaining -= r;
    return r;
}

static qint32 readRawPacket(void *opaque, quint8 *buf, qint32 bufSize) {
    Stream *stream = (Stream*)opaque;
    if (stream) {
        qint32 len = stream->recvData(buf, bufSize);
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

void Stream::setVideoSocket(VideoSocket* videoSocket)
{
    m_videoSocket = videoSocket;
}

void Stream::setRecoder(Recorder *recorder)
{
    m_recorder = recorder;
}

qint32 Stream::recvData(quint8* buf, qint32 bufSize)
{
    if (!buf) {
        return 0;
    }
    if (m_videoSocket) {
        qint32 len = m_videoSocket->subThreadRecvData(buf, bufSize);
        return len;
    }
    return 0;
}

bool Stream::startDecode()
{
    if (!m_videoSocket) {
        return false;
    }
    m_quit = false;
    start();
    return true;
}

void Stream::stopDecode()
{
    m_quit = true;
    if (m_decoder) {
        m_decoder->interrupt();
    }
    wait();
}

Stream::ReceiverState *Stream::getReceiverState()
{
    return &m_receiverState;
}

void Stream::run()
{
    unsigned char *decoderBuffer = Q_NULLPTR;
    AVIOContext *avioCtx = Q_NULLPTR;
    AVFormatContext *formatCtx = Q_NULLPTR;
    AVCodec *codec = Q_NULLPTR;
    AVCodecContext *codecCtx = Q_NULLPTR;
    ReadPacketFunc readPacket = Q_NULLPTR;
    bool isFormatCtxOpen = false;

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

    if (m_decoder && !m_decoder->open(codec)) {
        qCritical("Could not open m_decoder");
        goto runQuit;
    }

    if (m_recorder && !m_recorder->open(codec)) {
        qCritical("Could not open recorder");
        goto runQuit;
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = Q_NULLPTR;
    packet.size = 0;

    while (!av_read_frame(formatCtx, &packet)) {
        if (m_quit) {
            // if the stream is stopped, the socket had been shutdown, so the
            // last packet is probably corrupted (but not detected as such by
            // FFmpeg) and will not be decoded correctly
            av_packet_unref(&packet);
            goto runQuit;
        }
        if (m_decoder && !m_decoder->push(&packet)) {
            av_packet_unref(&packet);
            goto runQuit;
        }
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
        av_free(avioCtx->buffer);
        av_freep(&avioCtx);
    }
    if (formatCtx && isFormatCtxOpen) {
        avformat_close_input(&formatCtx);
    }
    if (formatCtx) {
        avformat_free_context(formatCtx);
    }
    if (m_decoder) {
        m_decoder->close();
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
    }

    emit onStreamStop();
}
