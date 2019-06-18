#include <QDebug>
#include <QFileInfo>

#include "compat.h"
#include "recorder.h"

static const AVRational SCRCPY_TIME_BASE = {1, 1000000}; // timestamps in us

Recorder::Recorder(const QString& fileName)
    : m_fileName(fileName)
    , m_format(guessRecordFormat(fileName))
{

}

Recorder::~Recorder()
{

}

void Recorder::setFrameSize(const QSize &declaredFrameSize)
{
    m_declaredFrameSize = declaredFrameSize;
}

void Recorder::setFormat(Recorder::RecorderFormat format)
{
    m_format = format;
}

bool Recorder::open(const AVCodec* inputCodec)
{
    QString formatName = recorderGetFormatName(m_format);
    Q_ASSERT(!formatName.isEmpty());
    const AVOutputFormat* format = findMuxer(formatName.toUtf8());
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

    m_formatCtx->oformat = (AVOutputFormat*)format;

    AVStream* outStream = avformat_new_stream(m_formatCtx, inputCodec);
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

    int ret = avio_open(&m_formatCtx->pb, m_fileName.toUtf8().toStdString().c_str(),
                        AVIO_FLAG_WRITE);
    if (ret < 0) {
        char errorbuf[255] = { 0 };
        av_strerror(ret, errorbuf, 254);
        qCritical(QString("Failed to open output file: %1 %2").arg(errorbuf).arg(m_fileName).toUtf8().toStdString().c_str());
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
        int ret = av_write_trailer(m_formatCtx);
        if (ret < 0) {
            qCritical(QString("Failed to write trailer to %1").arg(m_fileName).toUtf8().toStdString().c_str());
        } else {
            qInfo(QString("success record %1").arg(m_fileName).toStdString().c_str());
        }
        avio_close(m_formatCtx->pb);
        avformat_free_context(m_formatCtx);
        m_formatCtx = Q_NULLPTR;
    }
}

bool Recorder::write(AVPacket *packet)
{
    if (!m_headerWritten) {
        bool ok = recorderWriteHeader(packet);
        if (!ok) {
            return false;
        }
        m_headerWritten = true;
    }
    recorderRescalePacket(packet);
    return av_write_frame(m_formatCtx, packet) >= 0;
}

const AVOutputFormat *Recorder::findMuxer(const char* name)
{
#ifdef QTSCRCPY_LAVF_HAS_NEW_MUXER_ITERATOR_API
    void* opaque = Q_NULLPTR;
#endif
    const AVOutputFormat* outFormat = Q_NULLPTR;
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

bool Recorder::recorderWriteHeader(const AVPacket* packet)
{
    AVStream *ostream = m_formatCtx->streams[0];
    quint8* extradata = (quint8*)av_malloc(packet->size * sizeof(quint8));
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
        free(extradata);
        avio_close(m_formatCtx->pb);
        avformat_free_context(m_formatCtx);
        return false;
    }
    return true;
}

void Recorder::recorderRescalePacket(AVPacket* packet)
{
    AVStream *ostream = m_formatCtx->streams[0];
    av_packet_rescale_ts(packet, SCRCPY_TIME_BASE, ostream->time_base);
}

QString Recorder::recorderGetFormatName(Recorder::RecorderFormat format)
{
    switch (format) {
    case RECORDER_FORMAT_MP4: return "mp4";
    case RECORDER_FORMAT_MKV: return "matroska";
    default: return "";
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
