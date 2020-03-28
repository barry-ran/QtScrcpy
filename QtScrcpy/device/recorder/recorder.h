#ifndef RECORDER_H
#define RECORDER_H
#include <QMutex>
#include <QQueue>
#include <QSize>
#include <QString>
#include <QThread>
#include <QWaitCondition>

extern "C"
{
#include "libavformat/avformat.h"
}

class Recorder : public QThread
{
    Q_OBJECT
public:
    enum RecorderFormat
    {
        RECORDER_FORMAT_NULL = 0,
        RECORDER_FORMAT_MP4,
        RECORDER_FORMAT_MKV,
    };

    Recorder(const QString &fileName, QObject *parent = Q_NULLPTR);
    virtual ~Recorder();

    void setFrameSize(const QSize &declaredFrameSize);
    void setFormat(Recorder::RecorderFormat format);
    bool open(const AVCodec *inputCodec);
    void close();
    bool write(AVPacket *packet);
    bool startRecorder();
    void stopRecorder();
    bool push(const AVPacket *packet);

private:
    const AVOutputFormat *findMuxer(const char *name);
    bool recorderWriteHeader(const AVPacket *packet);
    void recorderRescalePacket(AVPacket *packet);
    QString recorderGetFormatName(Recorder::RecorderFormat format);
    RecorderFormat guessRecordFormat(const QString &fileName);

private:
    AVPacket *packetNew(const AVPacket *packet);
    void packetDelete(AVPacket *packet);
    void queueClear();

protected:
    void run();

private:
    QString m_fileName = "";
    AVFormatContext *m_formatCtx = Q_NULLPTR;
    QSize m_declaredFrameSize;
    bool m_headerWritten = false;
    RecorderFormat m_format = RECORDER_FORMAT_NULL;
    QMutex m_mutex;
    QWaitCondition m_recvDataCond;
    bool m_stopped = false; // set on recorder_stop() by the stream reader
    bool m_failed = false;  // set on packet write failure
    QQueue<AVPacket *> m_queue;
    // we can write a packet only once we received the next one so that we can
    // set its duration (next_pts - current_pts)
    // "previous" is only accessed from the recorder thread, so it does not
    // need to be protected by the mutex
    AVPacket *m_previous = Q_NULLPTR;
};

#endif // RECORDER_H
