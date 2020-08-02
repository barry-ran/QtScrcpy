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
    Controller(QString gameScript = "", QObject *parent = Q_NULLPTR);
    virtual ~Controller();

    void setControlSocket(QTcpSocket *controlSocket);
    void postControlMsg(ControlMsg *controlMsg);
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
    void onExpandNotificationPanel();
    void onCollapseNotificationPanel();
    void onSetScreenPowerMode(ControlMsg::ScreenPowerMode mode);

    // for input convert
    void onMouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize);
    void onWheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize);
    void onKeyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize);

    // turn the screen on if it was off, press BACK otherwise
    void onPostBackOrScreenOn();
    void onRequestDeviceClipboard();
    void onSetDeviceClipboard();
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
    QPointer<QTcpSocket> m_controlSocket;
    QPointer<Receiver> m_receiver;
    QPointer<InputConvertBase> m_inputConvert;
};

#endif // CONTROLLER_H
