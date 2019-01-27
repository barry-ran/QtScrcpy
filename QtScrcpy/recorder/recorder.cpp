#include <QDebug>

#include "recorder.h"

Recorder::Recorder(const QString& fileName)
    : m_fileName(fileName)
{

}

Recorder::~Recorder()
{

}

void Recorder::setFrameSize(const QSize &declaredFrameSize)
{
    m_declaredFrameSize = declaredFrameSize;
}

bool Recorder::open(AVCodec *inputCodec)
{
    const AVOutputFormat* mp4 = findMp4Muxer();
    if (!mp4) {
        qCritical("Could not find mp4 muxer");
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

    m_formatCtx->oformat = (AVOutputFormat*)mp4;

    AVStream* outStream = avformat_new_stream(m_formatCtx, inputCodec);
    if (!outStream) {
        avformat_free_context(m_formatCtx);
        m_formatCtx = Q_NULLPTR;
        return false;
    }

    // In ffmpeg/doc/APIchanges:
    // 2016-04-11 - 6f69f7a / 9200514 - lavf 57.33.100 / 57.5.0 - avformat.h
    // Add AVStream.codecpar, deprecate AVStream.codec.
#if (LIBAVFORMAT_VERSION_MICRO >= 100 /* FFmpeg */ && \
    LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 33, 100)) \
    || (LIBAVFORMAT_VERSION_MICRO < 100 && /* Libav */ \
    LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 5, 0))
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
    // timestamps in us
    outStream->time_base.num = 1;
    outStream->time_base.den = 1000000;

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

    ret = avformat_write_header(m_formatCtx, Q_NULLPTR);
    if (ret < 0) {
        qCritical(QString("Failed to write header to %1").arg(m_fileName).toUtf8().toStdString().c_str());
        avio_closep(&m_formatCtx->pb);
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
    return av_write_frame(m_formatCtx, packet) >= 0;
}

const AVOutputFormat *Recorder::findMp4Muxer()
{
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(58, 9, 100)
    void* opaque = Q_NULLPTR;
#endif
    const AVOutputFormat* outFormat = Q_NULLPTR;
    do {
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(58, 9, 100)
        outFormat = av_muxer_iterate(&opaque);
#else
        outFormat = av_oformat_next(outFormat);
#endif
        // until null or with name "mp4"
    } while (outFormat && strcmp(outFormat->name, "mp4"));
    return outFormat;
}
