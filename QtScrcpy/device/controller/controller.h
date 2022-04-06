
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

public slots:
    void onPostGoBack();
    void onPostGoHome();
    void onPostGoMenu();
    void onPostAppSwitch();
    void onPostPower();
    void onPostVolumeUp();
    void onPostVolumeDown();
    void onCopy();
    void onCut();
    void onExpandNotificationPanel();
    void onCollapsePanel();
    void onSetScreenPowerMode(ControlMsg::ScreenPowerMode mode);

    // for input convert
    void onMouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize);
    void onWheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize);
    void onKeyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize);

    // turn the screen on if it was off, press BACK otherwise
    // If the screen is off, it is turned on only on down
    void onPostBackOrScreenOn(bool down);
    void onRequestDeviceClipboard();
    void onGetDeviceClipboard(bool cut = false);
    void onSetDeviceClipboard(bool pause = true);
    void onClipboardPaste();
    void onPostTextInput(QString &text);

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
