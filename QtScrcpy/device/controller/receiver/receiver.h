#ifndef RECEIVER_H
#define RECEIVER_H

#include <QPointer>

class QTcpSocket;
class DeviceMsg;
class Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Receiver(QObject *parent = Q_NULLPTR);
    virtual ~Receiver();

    void setControlSocket(QTcpSocket *controlSocket);

public slots:
    void onReadyRead();

protected:
    void processMsg(DeviceMsg *deviceMsg);

private:
    QPointer<QTcpSocket> m_controlSocket;
};

#endif // RECEIVER_H
