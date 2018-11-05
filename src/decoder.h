#ifndef DECODER_H
#define DECODER_H
#include <functional>
#include <QThread>
#include <QPointer>
#include <QMutex>

#include "devicesocket.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class Frames;

class Decoder : public QThread
{
    Q_OBJECT
public:
    Decoder();
    virtual ~Decoder();

public:
    static bool init();
    static void deInit();    

    void setFrames(Frames* frames);
    void setDeviceSocket(DeviceSocket* deviceSocket);
    qint32 recvData(quint8* buf, qint32 bufSize);
    bool startDecode();
    void stopDecode();

signals:
    void onNewFrame();
    void onDecodeStop();

protected:
    void run();
    void pushFrame();

private:
    QPointer<DeviceSocket> m_deviceSocket;
    QMutex m_mutex;
    bool m_quit = false;
    Frames* m_frames;
};

#endif // DECODER_H
