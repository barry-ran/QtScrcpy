#include <QApplication>
#include <QClipboard>

#include "controller.h"
#include "videosocket.h"
#include "controlmsg.h"
#include "receiver.h"
#include "inputconvertgame.h"

Controller::Controller(QString gameScript, QObject* parent) : QObject(parent)
{
    m_receiver = new Receiver(this);
    Q_ASSERT(m_receiver);

    updateScript(gameScript);
    Q_ASSERT(m_inputConvert);
    connect(m_inputConvert, &InputConvertBase::grabCursor, this, &Controller::grabCursor);
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
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_INJECT_TOUCH);
    controlMsg->setInjectTouchMsgData(POINTER_ID_MOUSE, AMOTION_EVENT_ACTION_DOWN, AMOTION_EVENT_BUTTON_PRIMARY, rc, 1.0f);
    postControlMsg(controlMsg);
}

void Controller::updateScript(QString gameScript)
{
    if (!gameScript.isEmpty()) {
        InputConvertGame* convertgame = new InputConvertGame(this);
        convertgame->loadKeyMap(gameScript);
         m_inputConvert = convertgame;
    } else {
         m_inputConvert = new InputConvertNormal(this);
    }

}

void Controller::postTurnOn()
{
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_BACK_OR_SCREEN_ON);
    if (!controlMsg) {
        return;
    }
    postControlMsg(controlMsg);
}

void Controller::postGoHome()
{
    postKeyCodeClick(AKEYCODE_HOME);
}

void Controller::postGoMenu()
{
    postKeyCodeClick(AKEYCODE_MENU);
}

void Controller::postGoBack()
{
    postKeyCodeClick(AKEYCODE_BACK);
}

void Controller::postAppSwitch()
{
    postKeyCodeClick(AKEYCODE_APP_SWITCH);
}

void Controller::postPower()
{
    postKeyCodeClick(AKEYCODE_POWER);
}

void Controller::postVolumeUp()
{
    postKeyCodeClick(AKEYCODE_VOLUME_UP);
}

void Controller::postVolumeDown()
{
    postKeyCodeClick(AKEYCODE_VOLUME_DOWN);
}

void Controller::expandNotificationPanel()
{
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_EXPAND_NOTIFICATION_PANEL);
    if (!controlMsg) {
        return;
    }
    postControlMsg(controlMsg);
}

void Controller::collapseNotificationPanel()
{
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_COLLAPSE_NOTIFICATION_PANEL);
    if (!controlMsg) {
        return;
    }
    postControlMsg(controlMsg);
}

void Controller::requestDeviceClipboard()
{
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_GET_CLIPBOARD);
    if (!controlMsg) {
        return;
    }
    postControlMsg(controlMsg);
}

void Controller::setDeviceClipboard()
{
    QClipboard *board = QApplication::clipboard();
    QString text = board->text();
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_SET_CLIPBOARD);
    if (!controlMsg) {
        return;
    }
    controlMsg->setSetClipboardMsgData(text);
    postControlMsg(controlMsg);
}

void Controller::clipboardPaste()
{
    QClipboard *board = QApplication::clipboard();
    QString text = board->text();
    postTextInput(text);
}

void Controller::postTextInput(QString& text)
{
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_INJECT_TEXT);
    if (!controlMsg) {
        return;
    }
    controlMsg->setInjectTextMsgData(text);
    postControlMsg(controlMsg);
}

void Controller::setScreenPowerMode(ControlMsg::ScreenPowerMode mode)
{
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_SET_SCREEN_POWER_MODE);
    if (!controlMsg) {
        return;
    }
    controlMsg->setSetScreenPowerModeData(mode);
    postControlMsg(controlMsg);
}

void Controller::screenShot()
{
    // TODO:
    qDebug() << "screen shot";
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

void Controller::postKeyCodeClick(AndroidKeycode keycode)
{
    ControlMsg* controlEventDown = new ControlMsg(ControlMsg::CMT_INJECT_KEYCODE);
    if (!controlEventDown) {
        return;
    }
    controlEventDown->setInjectKeycodeMsgData(AKEY_EVENT_ACTION_DOWN, keycode, AMETA_NONE);
    postControlMsg(controlEventDown);

    ControlMsg* controlEventUp = new ControlMsg(ControlMsg::CMT_INJECT_KEYCODE);
    if (!controlEventUp) {
        return;
    }
    controlEventUp->setInjectKeycodeMsgData(AKEY_EVENT_ACTION_UP, keycode, AMETA_NONE);
    postControlMsg(controlEventUp);
}
