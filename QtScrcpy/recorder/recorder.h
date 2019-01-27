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
    Recorder(const QString& fileName);
    virtual ~Recorder();

    void setFrameSize(const QSize& declaredFrameSize);
    bool open(AVCodec* inputCodec);
    void close();
    bool write(AVPacket* packet);

private:
    const AVOutputFormat* findMp4Muxer();

private:
    QString m_fileName = "";
    AVFormatContext* m_formatCtx = Q_NULLPTR;
    QSize m_declaredFrameSize;
};

#endif // RECORDER_H
