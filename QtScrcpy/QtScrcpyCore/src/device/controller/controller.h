
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QPointer>

#include "inputconvertbase.h"

class QTcpSocket;
class Receiver;
class InputConvertBase;
class DeviceMsg;
class Controller : public QObject
{
    Q_OBJECT
public:
    Controller(std::function<qint64(const QByteArray&)> sendData, QString gameScript = "", QObject *parent = Q_NULLPTR);
    virtual ~Controller();

    void postControlMsg(ControlMsg *controlMsg);
    void recvDeviceMsg(DeviceMsg *deviceMsg);
    void test(QRect rc);

    void updateScript(QString gameScript = "");
    bool isCurrentCustomKeymap();

    void postGoBack();
    void postGoHome();
    void postGoMenu();
    void postAppSwitch();
    void postPower();
    void postVolumeUp();
    void postVolumeDown();
    void copy();
    void cut();
    void expandNotificationPanel();
    void collapsePanel();
    void setScreenPowerMode(ControlMsg::ScreenPowerMode mode);

    // for input convert
    void mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize);
    void wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize);
    void keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize);

    // turn the screen on if it was off, press BACK otherwise
    // If the screen is off, it is turned on only on down
    void postBackOrScreenOn(bool down);
    void requestDeviceClipboard();
    void getDeviceClipboard(bool cut = false);
    void setDeviceClipboard(bool pause = true);
    void clipboardPaste();
    void postTextInput(QString &text);

signals:
    void grabCursor(bool grab);

protected:
    bool event(QEvent *event);

private:
    bool sendControl(const QByteArray &buffer);
    void postKeyCodeClick(AndroidKeycode keycode);

private:
    QPointer<Receiver> m_receiver;
    QPointer<InputConvertBase> m_inputConvert;
    std::function<qint64(const QByteArray&)> m_sendData = Q_NULLPTR;
};

#endif // CONTROLLER_H
