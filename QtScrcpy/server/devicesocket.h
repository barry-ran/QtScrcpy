#ifndef DEVICESOCKET_H
#define DEVICESOCKET_H

#include <QEvent>
#include <QTcpSocket>
#include <QMutex>
#include <QWaitCondition>

class DeviceSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit DeviceSocket(QObject *parent = nullptr);
    virtual ~DeviceSocket();

    qint32 subThreadRecvData(quint8* buf, qint32 bufSize);

protected:
    bool event(QEvent *event);

protected slots:
    void onReadyRead();
    void quitNotify();

private:
    QMutex m_mutex;
    QWaitCondition m_recvDataCond;
    bool m_recvData = false;
    quint8* m_buffer = Q_NULLPTR;
    qint32 m_bufferSize = 0;
    qint32 m_dataSize = 0;
    bool m_quit = false;
};

#endif // DEVICESOCKET_H
