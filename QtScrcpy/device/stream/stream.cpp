#include <QDebug>
#include <QTime>

#include "compat.h"
#include "decoder.h"
#include "recorder.h"
#include "stream.h"
#include "videosocket.h"

#define BUFSIZE 0x10000
#define HEADER_SIZE 12
#define NO_PTS UINT64_MAX

typedef qint32 (*ReadPacketFunc)(void *, quint8 *, qint32);

Stream::Stream(QObject *parent) : QThread(parent) {}

Stream::~Stream() {}

static void avLogCallback(void *avcl, int level, const char *fmt, va_list vl)
{
    Q_UNUSED(avcl)
    Q_UNUSED(vl)

    QString localFmt = QString::fromUtf8(fmt);
    localFmt.prepend("[FFmpeg] ");
    switch (level) {
    case AV_LOG_PANIC:
    case AV_LOG_FATAL:
        qFatal("%s", localFmt.toUtf8().data());
        break;
    case AV_LOG_ERROR:
        qCritical() << localFmt.toUtf8();
        break;
    case AV_LOG_WARNING:
        qWarning() << localFmt.toUtf8();
        break;
    case AV_LOG_INFO:
        qInfo() << localFmt.toUtf8();
        break;
    case AV_LOG_DEBUG:
        // qDebug() << localFmt.toUtf8();
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

void Stream::setDecoder(Decoder *decoder)
{
    m_decoder = decoder;
}

static quint32 bufferRead32be(quint8 *buf)
{
    return static_cast<quint32>((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
}

static quint64 bufferRead64be(quint8 *buf)
{
    quint32 msb = bufferRead32be(buf);
    quint32 lsb = bufferRead32be(&buf[4]);
    return (static_cast<quint64>(msb) << 32) | lsb;
}

void Stream::setVideoSocket(VideoSocket *videoSocket)
{
    m_videoSocket = videoSocket;
}

void Stream::setRecoder(Recorder *recorder)
{
    m_recorder = recorder;
}

qint32 Stream::recvData(quint8 *buf, qint32 bufSize)
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
    start();
    return true;
}

void Stream::stopDecode()
{
    if (m_decoder) {
        m_decoder->interrupt();
    }
    wait();
}

void Stream::run()
{
    AVCodec *codec = Q_NULLPTR;
    m_codecCtx = Q_NULLPTR;
    m_parser = Q_NULLPTR;

    // codec
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        qCritical("H.264 decoder not found");
        goto runQuit;
    }

    // codeCtx
    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        qCritical("Could not allocate codec context");
        goto runQuit;
    }

    if (m_decoder && !m_decoder->open(codec)) {
        qCritical("Could not open m_decoder");
        goto runQuit;
    }

    if (m_recorder) {
        if (!m_recorder->open(codec)) {
            qCritical("Could not open recorder");
            goto runQuit;
        }

        if (!m_recorder->startRecorder()) {
            qCritical("Could not start recorder");
            goto runQuit;
        }
    }

    m_parser = av_parser_init(AV_CODEC_ID_H264);
    if (!m_parser) {
        qCritical("Could not initialize parser");
        goto runQuit;
    }

    // We must only pass complete frames to av_parser_parse2()!
    // It's more complicated, but this allows to reduce the latency by 1 frame!
    m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;

    for (;;) {
        AVPacket packet;
        bool ok = recvPacket(&packet);
        if (!ok) {
            // end of stream
            break;
        }

        ok = pushPacket(&packet);
        av_packet_unref(&packet);
        if (!ok) {
            // cannot process packet (error already logged)
            break;
        }
    }

    qDebug("End of frames");

    if (m_hasPending) {
        av_packet_unref(&m_pending);
    }

    av_parser_close(m_parser);

runQuit:
    if (m_recorder) {
        if (m_recorder->isRunning()) {
            m_recorder->stopRecorder();
            m_recorder->wait();
        }
        m_recorder->close();
    }
    if (m_decoder) {
        m_decoder->close();
    }
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }

    emit onStreamStop();
}

bool Stream::recvPacket(AVPacket *packet)
{
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

    quint8 header[HEADER_SIZE];
    qint32 r = recvData(header, HEADER_SIZE);
    if (r < HEADER_SIZE) {
        return false;
    }

    quint64 pts = bufferRead64be(header);
    quint32 len = bufferRead32be(&header[8]);
    Q_ASSERT(pts == NO_PTS || (pts & 0x8000000000000000) == 0);
    Q_ASSERT(len);

    if (av_new_packet(packet, static_cast<int>(len))) {
        qCritical("Could not allocate packet");
        return false;
    }

    r = recvData(packet->data, static_cast<qint32>(len));
    if (r < 0 || static_cast<quint32>(r) < len) {
        av_packet_unref(packet);
        return false;
    }

    packet->pts = pts != NO_PTS ? static_cast<int64_t>(pts) : static_cast<int64_t>(AV_NOPTS_VALUE);

    return true;
}

bool Stream::pushPacket(AVPacket *packet)
{
    bool isConfig = packet->pts == AV_NOPTS_VALUE;

    // A config packet must not be decoded immetiately (it contains no
    // frame); instead, it must be concatenated with the future data packet.
    if (m_hasPending || isConfig) {
        qint32 offset;
        if (m_hasPending) {
            offset = m_pending.size;
            if (av_grow_packet(&m_pending, packet->size)) {
                qCritical("Could not grow packet");
                return false;
            }
        } else {
            offset = 0;
            if (av_new_packet(&m_pending, packet->size)) {
                qCritical("Could not create packet");
                return false;
            }
            m_hasPending = true;
        }

        memcpy(m_pending.data + offset, packet->data, static_cast<unsigned int>(packet->size));

        if (!isConfig) {
            // prepare the concat packet to send to the decoder
            m_pending.pts = packet->pts;
            m_pending.dts = packet->dts;
            m_pending.flags = packet->flags;
            packet = &m_pending;
        }
    }

    if (isConfig) {
        // config packet
        bool ok = processConfigPacket(packet);
        if (!ok) {
            return false;
        }
    } else {
        // data packet
        bool ok = parse(packet);

        if (m_hasPending) {
            // the pending packet must be discarded (consumed or error)
            m_hasPending = false;
            av_packet_unref(&m_pending);
        }

        if (!ok) {
            return false;
        }
    }
    return true;
}

bool Stream::processConfigPacket(AVPacket *packet)
{
    if (m_recorder && !m_recorder->push(packet)) {
        qCritical("Could not send config packet to recorder");
        return false;
    }
    return true;
}

bool Stream::parse(AVPacket *packet)
{
    quint8 *inData = packet->data;
    int inLen = packet->size;
    quint8 *outData = Q_NULLPTR;
    int outLen = 0;
    int r = av_parser_parse2(m_parser, m_codecCtx, &outData, &outLen, inData, inLen, AV_NOPTS_VALUE, AV_NOPTS_VALUE, -1);

    // PARSER_FLAG_COMPLETE_FRAMES is set
    Q_ASSERT(r == inLen);
    (void)r;
    Q_ASSERT(outLen == inLen);

    if (m_parser->key_frame == 1) {
        packet->flags |= AV_PKT_FLAG_KEY;
    }

    bool ok = processFrame(packet);
    if (!ok) {
        qCritical("Could not process frame");
        return false;
    }

    return true;
}

bool Stream::processFrame(AVPacket *packet)
{
    if (m_decoder && !m_decoder->push(packet)) {
        return false;
    }

    if (m_recorder) {
        packet->dts = packet->pts;

        if (!m_recorder->push(packet)) {
            qCritical("Could not send packet to recorder");
            return false;
        }
    }

    return true;
}
