#ifndef DECODER_H
#define DECODER_H
#include <functional>
#include <QThread>
#include <QTcpSocket>
#include <QPointer>
#include <QImage>

#include "convert.h"
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
    virtual ~Decoder();

public:
    static bool init();
    static void deInit();    

    void setDeviceSocket(QTcpSocket* deviceSocket);
    qint32 recvData(quint8* buf, qint32 bufSize);
    bool startDecode();
    void stopDecode();

signals:
    void getOneFrame(quint8* bufferY, quint8* bufferU, quint8* bufferV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV);

protected:
    void run();

private:
    QPointer<QTcpSocket> m_deviceSocket = Q_NULLPTR;
    bool m_quit = false;
    Convert m_conver;
};

#endif // DECODER_H
