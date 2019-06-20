#include <QCoreApplication>

#include "controller.h"
#include "videosocket.h"
#include "controlmsg.h"
#include "receiver.h"

Controller::Controller(QObject* parent) : QObject(parent)
{
    m_receiver = new Receiver(this);
    Q_ASSERT(m_receiver);

    m_inputConvert = new InputConvertGame(this);
    Q_ASSERT(m_inputConvert);
    connect(m_inputConvert, &InputConvertGame::grabCursor, this, &Controller::grabCursor);
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
    m_receiver->setControlSocket(controlSocket);
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

void Controller::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (m_inputConvert) {
        m_inputConvert->mouseEvent(from, frameSize, showSize);
    }
}

void Controller::wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (m_inputConvert) {
        m_inputConvert->wheelEvent(from, frameSize, showSize);
    }
}

void Controller::keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (m_inputConvert) {
        m_inputConvert->keyEvent(from, frameSize, showSize);
    }
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
