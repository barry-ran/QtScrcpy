#ifndef DECODER_H
#define DECODER_H
#include <functional>
#include <QThread>
#include <QTcpSocket>
#include <QPointer>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class Decoder : public QThread
{
    Q_OBJECT
public:
    Decoder();

public:
    static bool init();
    static void deInit();    

    void setDeviceSocket(QTcpSocket* deviceSocket);
    qint32 recvData(quint8* buf, qint32 bufSize);

protected:
    void run();

private:
    QPointer<QTcpSocket> m_deviceSocket = Q_NULLPTR;
};

#endif // DECODER_H
