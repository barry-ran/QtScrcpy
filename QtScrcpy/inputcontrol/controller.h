#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QPointer>

class QTcpSocket;
class ControlEvent;
class Controller : public QObject
{
    Q_OBJECT
public:
    Controller(QObject* parent = Q_NULLPTR);
    virtual ~Controller();

    void setControlSocket(QTcpSocket* controlSocket);
    void postControlEvent(ControlEvent* controlEvent);
    void test(QRect rc);

protected:
    bool event(QEvent *event);

private:
    bool sendControl(const QByteArray& buffer);

private:
    QPointer<QTcpSocket> m_controlSocket;
};

#endif // CONTROLLER_H
