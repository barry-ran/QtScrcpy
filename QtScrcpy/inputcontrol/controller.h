#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QPointer>

class QTcpSocket;
class ControlMsg;
class Receiver;
class Controller : public QObject
{
    Q_OBJECT
public:
    Controller(QObject* parent = Q_NULLPTR);
    virtual ~Controller();

    void setControlSocket(QTcpSocket* controlSocket);
    QTcpSocket* getControlSocket();
    void postControlMsg(ControlMsg* controlMsg);
    void test(QRect rc);

protected:
    bool event(QEvent *event);

private:
    bool sendControl(const QByteArray& buffer);

private:
    QPointer<QTcpSocket> m_controlSocket;
    QPointer<Receiver> m_receiver;
};

#endif // CONTROLLER_H
