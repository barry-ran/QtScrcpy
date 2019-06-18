#ifndef RECORDER_H
#define RECORDER_H
#include <QString>
#include <QSize>

extern "C"
{
#include "libavformat/avformat.h"
}

class Recorder
{
public:
    enum RecorderFormat {
        RECORDER_FORMAT_NULL = 0,
        RECORDER_FORMAT_MP4,
        RECORDER_FORMAT_MKV,
    };

    Recorder(const QString& fileName);
    virtual ~Recorder();

    void setFrameSize(const QSize& declaredFrameSize);
    void setFormat(Recorder::RecorderFormat format);
    bool open(const AVCodec* inputCodec);
    void close();
    bool write(AVPacket* packet);

private:
    const AVOutputFormat* findMuxer(const char* name);
    bool recorderWriteHeader(const AVPacket* packet);
    void recorderRescalePacket(AVPacket *packet);
    QString recorderGetFormatName(Recorder::RecorderFormat format);
    RecorderFormat guessRecordFormat(const QString& fileName);

private:
    QString m_fileName = "";
    AVFormatContext* m_formatCtx = Q_NULLPTR;
    QSize m_declaredFrameSize;
    bool m_headerWritten = false;
    RecorderFormat m_format = RECORDER_FORMAT_NULL;
};

#endif // RECORDER_H
