#ifndef DECODER_H
#define DECODER_H

#include <QThread>
#include <QPointer>
#include <QMutex>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class VideoBuffer;
class DeviceSocket;
class Recorder;
class Decoder : public QThread
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

    Decoder();
    virtual ~Decoder();

public:
    static bool init();
    static void deInit();    

    void setVideoBuffer(VideoBuffer* vb);
    void setDeviceSocket(DeviceSocket* deviceSocket);
    void setRecoder(Recorder* recorder);
    qint32 recvData(quint8* buf, qint32 bufSize);
    bool startDecode();
    void stopDecode();
    ReceiverState* getReceiverState();

signals:
    void onNewFrame();
    void onDecodeStop();

protected:
    void run();
    void pushFrame();

private:
    QPointer<DeviceSocket> m_deviceSocket;
    bool m_quit = false;
    VideoBuffer* m_vb;

    // for recorder
    Recorder* m_recorder = Q_NULLPTR;
    ReceiverState m_receiverState;
};

#endif // DECODER_H
