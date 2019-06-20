#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QPointer>

#include "inputconvertgame.h"

class QTcpSocket;
class Receiver;
class Controller : public QObject
{
    Q_OBJECT
public:
    Controller(QObject* parent = Q_NULLPTR);
    virtual ~Controller();

    void setControlSocket(QTcpSocket* controlSocket);
    void postControlMsg(ControlMsg* controlMsg);
    void test(QRect rc);

    // for input convert
    void mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize);
    void wheelEvent(const QWheelEvent* from, const QSize& frameSize, const QSize& showSize);
    void keyEvent(const QKeyEvent* from, const QSize& frameSize, const QSize& showSize);

signals:
    void grabCursor(bool grab);

protected:
    bool event(QEvent *event);

private:
    bool sendControl(const QByteArray& buffer);

private:
    QPointer<QTcpSocket> m_controlSocket;
    QPointer<Receiver> m_receiver;
    QPointer<InputConvertBase> m_inputConvert;
};

#endif // CONTROLLER_H
