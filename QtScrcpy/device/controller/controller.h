#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QPointer>

#include "inputconvertbase.h"

class QTcpSocket;
class Receiver;
class InputConvertBase;
class Controller : public QObject
{
    Q_OBJECT
public:
    Controller(QString gameScript = "", QObject* parent = Q_NULLPTR);
    virtual ~Controller();

    void setControlSocket(QTcpSocket* controlSocket);    
    void postControlMsg(ControlMsg* controlMsg);
    void test(QRect rc);

    void updateScript(QString gameScript = "");

    // turn the screen on if it was off, press BACK otherwise
    void postTurnOn();
    void postGoHome();
    void postGoMenu();
    void postGoBack();
    void postAppSwitch();
    void postPower();
    void postVolumeUp();
    void postVolumeDown();
    void expandNotificationPanel();
    void collapseNotificationPanel();
    void requestDeviceClipboard();
    void setDeviceClipboard();
    void clipboardPaste();
    void postTextInput(QString& text);
    void setScreenPowerMode(ControlMsg::ScreenPowerMode mode);
    void screenShot();

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
    void postKeyCodeClick(AndroidKeycode keycode);    

private:
    QPointer<QTcpSocket> m_controlSocket;
    QPointer<Receiver> m_receiver;
    QPointer<InputConvertBase> m_inputConvert;
};

#endif // CONTROLLER_H
