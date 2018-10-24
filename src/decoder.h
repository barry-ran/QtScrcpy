#ifndef DECODER_H
#define DECODER_H
#include <functional>
#include <QThread>
#include <QTcpSocket>
#include <QPointer>
#include <QMutex>

//#include "convert.h"

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
    void setDeviceSocket(QTcpSocket* deviceSocket);
    qint32 recvData(quint8* buf, qint32 bufSize);
    bool startDecode();
    void stopDecode();

signals:
    void newFrame();

protected:
    void run();
    void pushFrame();

private:
    QPointer<QTcpSocket> m_deviceSocket = Q_NULLPTR;    
    QMutex m_mutex;
    bool m_quit = false;
    Frames* m_frames;
};

#endif // DECODER_H
