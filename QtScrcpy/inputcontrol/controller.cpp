#include <QCoreApplication>

#include "controller.h"
#include "videosocket.h"
#include "controlmsg.h"
#include "receiver.h"

Controller::Controller(QObject* parent) : QObject(parent)
{
    m_receiver = new Receiver(this);
}

Controller::~Controller()
{

}

void Controller::setControlSocket(QTcpSocket* controlSocket)
{
    if (m_controlSocket || !controlSocket) {
        return;
    }
    m_controlSocket = controlSocket;
    connect(controlSocket, &QTcpSocket::readyRead, m_receiver, &Receiver::onReadyRead);
}

QTcpSocket *Controller::getControlSocket()
{
    return m_controlSocket;
}

void Controller::postControlMsg(ControlMsg *controlMsg)
{
    if (controlMsg) {
        QCoreApplication::postEvent(this, controlMsg);
    }
}

void Controller::test(QRect rc)
{
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_INJECT_MOUSE);
    controlMsg->setInjectMouseMsgData(AMOTION_EVENT_ACTION_DOWN, AMOTION_EVENT_BUTTON_PRIMARY, rc);
    postControlMsg(controlMsg);
}

bool Controller::event(QEvent *event)
{
    if (event && event->type() == ControlMsg::Control) {
        ControlMsg* controlMsg = dynamic_cast<ControlMsg*>(event);
        if (controlMsg) {
            sendControl(controlMsg->serializeData());
        }
        return true;
    }
    return QObject::event(event);
}

bool Controller::sendControl(const QByteArray &buffer)
{
    if (buffer.isEmpty()) {
        return false;
    }
    qint32 len = 0;
    if (m_controlSocket) {
        len = m_controlSocket->write(buffer.data(), buffer.length());
    }
    return len == buffer.length() ? true : false;
}
