#ifndef STREAM_H
#define STREAM_H

#include <QThread>
#include <QPointer>
#include <QMutex>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class VideoSocket;
class Recorder;
class Decoder;
class Stream : public QThread
{
    Q_OBJECT
public:
    typedef struct FrameMeta {
        quint64 pts;
        struct FrameMeta* next;
    } FrameMeta;

    typedef struct ReceiverState {
        // meta (in order) for frames not consumed yet
        FrameMeta* frameMetaQueue;
        qint32 remaining; // remaining bytes to receive for the current frame
    } ReceiverState;

    Stream(QObject *parent = Q_NULLPTR);
    virtual ~Stream();

public:
    static bool init();
    static void deInit();

    void setDecoder(Decoder* vb);
    void setVideoSocket(VideoSocket* deviceSocket);
    void setRecoder(Recorder* recorder);
    qint32 recvData(quint8* buf, qint32 bufSize);
    bool startDecode();
    void stopDecode();
    ReceiverState* getReceiverState();

signals:
    void onStreamStop();

protected:
    void run();

private:
    QPointer<VideoSocket> m_videoSocket;
    std::atomic_bool m_quit;

    // for recorder
    Recorder* m_recorder = Q_NULLPTR;
    ReceiverState m_receiverState;
    Decoder* m_decoder = Q_NULLPTR;
};

#endif // STREAM_H
