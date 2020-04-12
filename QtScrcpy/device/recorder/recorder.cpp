#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>

#include "compat.h"
#include "recorder.h"

static const AVRational SCRCPY_TIME_BASE = { 1, 1000000 }; // timestamps in us

Recorder::Recorder(const QString &fileName, QObject *parent) : QThread(parent), m_fileName(fileName), m_format(guessRecordFormat(fileName)) {}

Recorder::~Recorder() {}

AVPacket *Recorder::packetNew(const AVPacket *packet)
{
    AVPacket *rec = new AVPacket;
    if (!rec) {
        return Q_NULLPTR;
    }

    // av_packet_ref() does not initialize all fields in old FFmpeg versions
    av_init_packet(rec);

    if (av_packet_ref(rec, packet)) {
        delete rec;
        return Q_NULLPTR;
    }
    return rec;
}

void Recorder::packetDelete(AVPacket *packet)
{
    av_packet_unref(packet);
    delete packet;
}

void Recorder::queueClear()
{
    while (!m_queue.isEmpty()) {
        packetDelete(m_queue.dequeue());
    }
}

void Recorder::setFrameSize(const QSize &declaredFrameSize)
{
    m_declaredFrameSize = declaredFrameSize;
}

void Recorder::setFormat(Recorder::RecorderFormat format)
{
    m_format = format;
}

bool Recorder::open(const AVCodec *inputCodec)
{
    QString formatName = recorderGetFormatName(m_format);
    Q_ASSERT(!formatName.isEmpty());
    const AVOutputFormat *format = findMuxer(formatName.toUtf8());
    if (!format) {
        qCritical("Could not find muxer");
        return false;
    }

    m_formatCtx = avformat_alloc_context();
    if (!m_formatCtx) {
        qCritical("Could not allocate output context");
        return false;
    }

    // contrary to the deprecated API (av_oformat_next()), av_muxer_iterate()
    // returns (on purpose) a pointer-to-const, but AVFormatContext.oformat
    // still expects a pointer-to-non-const (it has not be updated accordingly)
    // <https://github.com/FFmpeg/FFmpeg/commit/0694d8702421e7aff1340038559c438b61bb30dd>

    m_formatCtx->oformat = (AVOutputFormat *)format;

    QString comment = "Recorded by QtScrcpy " + QCoreApplication::applicationVersion();
    av_dict_set(&m_formatCtx->metadata, "comment", comment.toUtf8(), 0);

    AVStream *outStream = avformat_new_stream(m_formatCtx, inputCodec);
    if (!outStream) {
        avformat_free_context(m_formatCtx);
        m_formatCtx = Q_NULLPTR;
        return false;
    }

#ifdef QTSCRCPY_LAVF_HAS_NEW_CODEC_PARAMS_API
    outStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    outStream->codecpar->codec_id = inputCodec->id;
    outStream->codecpar->format = AV_PIX_FMT_YUV420P;
    outStream->codecpar->width = m_declaredFrameSize.width();
    outStream->codecpar->height = m_declaredFrameSize.height();
#else
    outStream->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    outStream->codec->codec_id = inputCodec->id;
    outStream->codec->pix_fmt = AV_PIX_FMT_YUV420P;
    outStream->codec->width = m_declaredFrameSize.width();
    outStream->codec->height = m_declaredFrameSize.height();
#endif

    int ret = avio_open(&m_formatCtx->pb, m_fileName.toUtf8().toStdString().c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        char errorbuf[255] = { 0 };
        av_strerror(ret, errorbuf, 254);
        qCritical() << QString("Failed to open output file: %1 %2").arg(errorbuf).arg(m_fileName).toUtf8().toStdString().c_str();
        // ostream will be cleaned up during context cleaning
        avformat_free_context(m_formatCtx);
        m_formatCtx = Q_NULLPTR;
        return false;
    }

    return true;
}

void Recorder::close()
{
    if (Q_NULLPTR != m_formatCtx) {
        if (m_headerWritten) {
            int ret = av_write_trailer(m_formatCtx);
            if (ret < 0) {
                qCritical() << QString("Failed to write trailer to %1").arg(m_fileName).toUtf8().toStdString().c_str();
                m_failed = true;
            } else {
                qInfo() << QString("success record %1").arg(m_fileName).toStdString().c_str();
            }
        } else {
            // the recorded file is empty
            m_failed = true;
        }
        avio_close(m_formatCtx->pb);
        avformat_free_context(m_formatCtx);
        m_formatCtx = Q_NULLPTR;
    }
}

bool Recorder::write(AVPacket *packet)
{
    if (!m_headerWritten) {
        if (packet->pts != AV_NOPTS_VALUE) {
            qCritical("The first packet is not a config packet");
            return false;
        }
        bool ok = recorderWriteHeader(packet);
        if (!ok) {
            return false;
        }
        m_headerWritten = true;
        return true;
    }

    if (packet->pts == AV_NOPTS_VALUE) {
        // ignore config packets
        return true;
    }

    recorderRescalePacket(packet);
    return av_write_frame(m_formatCtx, packet) >= 0;
}

const AVOutputFormat *Recorder::findMuxer(const char *name)
{
#ifdef QTSCRCPY_LAVF_HAS_NEW_MUXER_ITERATOR_API
    void *opaque = Q_NULLPTR;
#endif
    const AVOutputFormat *outFormat = Q_NULLPTR;
    do {
#ifdef QTSCRCPY_LAVF_HAS_NEW_MUXER_ITERATOR_API
        outFormat = av_muxer_iterate(&opaque);
#else
        outFormat = av_oformat_next(outFormat);
#endif
        // until null or with name "name"
    } while (outFormat && strcmp(outFormat->name, name));
    return outFormat;
}

bool Recorder::recorderWriteHeader(const AVPacket *packet)
{
    AVStream *ostream = m_formatCtx->streams[0];
    quint8 *extradata = (quint8 *)av_malloc(packet->size * sizeof(quint8));
    if (!extradata) {
        qCritical("Cannot allocate extradata");
        return false;
    }
    // copy the first packet to the extra data
    memcpy(extradata, packet->data, packet->size);

#ifdef QTSCRCPY_LAVF_HAS_NEW_CODEC_PARAMS_API
    ostream->codecpar->extradata = extradata;
    ostream->codecpar->extradata_size = packet->size;
#else
    ostream->codec->extradata = extradata;
    ostream->codec->extradata_size = packet->size;
#endif

    int ret = avformat_write_header(m_formatCtx, NULL);
    if (ret < 0) {
        qCritical("Failed to write header recorder file");
        return false;
    }
    return true;
}

void Recorder::recorderRescalePacket(AVPacket *packet)
{
    AVStream *ostream = m_formatCtx->streams[0];
    av_packet_rescale_ts(packet, SCRCPY_TIME_BASE, ostream->time_base);
}

QString Recorder::recorderGetFormatName(Recorder::RecorderFormat format)
{
    switch (format) {
    case RECORDER_FORMAT_MP4:
        return "mp4";
    case RECORDER_FORMAT_MKV:
        return "matroska";
    default:
        return "";
    }
}

Recorder::RecorderFormat Recorder::guessRecordFormat(const QString &fileName)
{
    if (4 > fileName.length()) {
        return Recorder::RECORDER_FORMAT_NULL;
    }
    QFileInfo fileInfo = QFileInfo(fileName);
    QString ext = fileInfo.suffix();
    if (0 == ext.compare("mp4")) {
        return Recorder::RECORDER_FORMAT_MP4;
    }
    if (0 == ext.compare("mkv")) {
        return Recorder::RECORDER_FORMAT_MKV;
    }

    return Recorder::RECORDER_FORMAT_NULL;
}

void Recorder::run()
{
    for (;;) {
        AVPacket *rec = Q_NULLPTR;
        {
            QMutexLocker locker(&m_mutex);
            while (!m_stopped && m_queue.isEmpty()) {
                m_recvDataCond.wait(&m_mutex);
            }

            // if stopped is set, continue to process the remaining events (to
            // finish the recording) before actually stopping
            if (m_stopped && m_queue.isEmpty()) {
                AVPacket *last = m_previous;
                if (last) {
                    // assign an arbitrary duration to the last packet
                    last->duration = 100000;
                    bool ok = write(last);
                    if (!ok) {
                        // failing to write the last frame is not very serious, no
                        // future frame may depend on it, so the resulting file
                        // will still be valid
                        qWarning("Could not record last packet");
                    }
                    packetDelete(last);
                }
                break;
            }

            rec = m_queue.dequeue();
        }

        // recorder->previous is only written from this thread, no need to lock
        AVPacket *previous = m_previous;
        m_previous = rec;

        if (!previous) {
            // we just received the first packet
            continue;
        }

        // config packets have no PTS, we must ignore them
        if (rec->pts != AV_NOPTS_VALUE && previous->pts != AV_NOPTS_VALUE) {
            // we now know the duration of the previous packet
            previous->duration = rec->pts - previous->pts;
        }

        bool ok = write(previous);
        packetDelete(previous);
        if (!ok) {
            qCritical("Could not record packet");
            QMutexLocker locker(&m_mutex);
            m_failed = true;
            // discard pending packets
            queueClear();
            break;
        }
    }

    qDebug("Recorder thread ended");
}

bool Recorder::startRecorder()
{
    start();
    return true;
}

void Recorder::stopRecorder()
{
    QMutexLocker locker(&m_mutex);
    m_stopped = true;
    m_recvDataCond.wakeOne();
}

bool Recorder::push(const AVPacket *packet)
{
    QMutexLocker locker(&m_mutex);
    Q_ASSERT(!m_stopped);

    if (m_failed) {
        // reject any new packet (this will stop the stream)
        return false;
    }

    AVPacket *rec = packetNew(packet);
    if (rec) {
        m_queue.enqueue(rec);
        m_recvDataCond.wakeOne();
    }
    return rec != Q_NULLPTR;
}
