#ifndef RECORDER_H
#define RECORDER_H
#include <QString>
#include <QSize>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

extern "C"
{
#include "libavformat/avformat.h"
}

class Recorder : public QThread
{
    Q_OBJECT
public:
    enum RecorderFormat {
        RECORDER_FORMAT_NULL = 0,
        RECORDER_FORMAT_MP4,
        RECORDER_FORMAT_MKV,
    };

    Recorder(const QString& fileName, QObject *parent = Q_NULLPTR);
    virtual ~Recorder();

    void setFrameSize(const QSize& declaredFrameSize);
    void setFormat(Recorder::RecorderFormat format);
    bool open(const AVCodec* inputCodec);
    void close();
    bool write(AVPacket* packet);
    bool startRecorder();
    void stopRecorder();
    bool push(const AVPacket *packet);

private:
    const AVOutputFormat* findMuxer(const char* name);
    bool recorderWriteHeader(const AVPacket* packet);
    void recorderRescalePacket(AVPacket *packet);
    QString recorderGetFormatName(Recorder::RecorderFormat format);
    RecorderFormat guessRecordFormat(const QString& fileName);

private:
    struct RecordPacket {
        AVPacket packet;
        RecordPacket *next;
    };

    struct RecorderQueue {
        RecordPacket *first = Q_NULLPTR;
        RecordPacket *last = Q_NULLPTR; // undefined if first is NULL
    };

    Recorder::RecordPacket* packetNew(const AVPacket *packet);
    void packetDelete(Recorder::RecordPacket *rec);
    void queueInit(Recorder::RecorderQueue *queue);
    bool queueIsEmpty(Recorder::RecorderQueue *queue);
    bool queuePush(Recorder::RecorderQueue *queue, const AVPacket *packet);
    Recorder::RecordPacket* queueTake(Recorder::RecorderQueue *queue);
    void queueClear(Recorder::RecorderQueue *queue);

protected:
    void run();

private:
    QString m_fileName = "";
    AVFormatContext* m_formatCtx = Q_NULLPTR;
    QSize m_declaredFrameSize;
    bool m_headerWritten = false;
    RecorderFormat m_format = RECORDER_FORMAT_NULL;
    QMutex m_mutex;
    QWaitCondition m_recvDataCond;
    bool m_stopped = false; // set on recorder_stop() by the stream reader
    bool m_failed = false; // set on packet write failure
    RecorderQueue m_queue;
};

#endif // RECORDER_H
