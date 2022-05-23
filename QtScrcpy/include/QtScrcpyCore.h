#pragma once
#include <QMouseEvent>

#include "QtScrcpyCoreDef.h"

namespace qsc {

class DeviceObserver {
protected:
    DeviceObserver() {

    }
    virtual ~DeviceObserver() {

    }

public:
    virtual void onFrame(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV, int linesizeY, int linesizeU, int linesizeV) {}
    virtual void updateFPS(quint32 fps) {}
    virtual void grabCursor(bool grab) {}
};

class IDevice : public QObject {
    Q_OBJECT
public:
    IDevice(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IDevice(){}

signals:
    void deviceConnected(bool success, const QString& serial, const QString& deviceName, const QSize& size);
    void deviceDisconnected(QString serial);

public:
    virtual void setUserData(void* data) = 0;
    virtual void* getUserData() = 0;
    virtual void registerDeviceObserver(DeviceObserver* observer) = 0;
    virtual void deRegisterDeviceObserver(DeviceObserver* observer) = 0;

    virtual bool connectDevice() = 0;
    virtual void disconnectDevice() = 0;

    virtual void mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize) = 0;
    virtual void wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize) = 0;
    virtual void keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize) = 0;

    virtual void postGoBack() = 0;
    virtual void postGoHome() = 0;
    virtual void postGoMenu() = 0;
    virtual void postAppSwitch() = 0;
    virtual void postPower() = 0;
    virtual void postVolumeUp() = 0;
    virtual void postVolumeDown() = 0;
    virtual void postCopy() = 0;
    virtual void postCut() = 0;
    virtual void setScreenPowerMode(bool open) = 0;
    virtual void expandNotificationPanel() = 0;
    virtual void collapsePanel() = 0;
    virtual void postBackOrScreenOn(bool down) = 0;
    virtual void postTextInput(QString &text) = 0;
    virtual void requestDeviceClipboard() = 0;
    virtual void setDeviceClipboard(bool pause = true) = 0;
    virtual void clipboardPaste() = 0;
    virtual void pushFileRequest(const QString &file, const QString &devicePath = "") = 0;
    virtual void installApkRequest(const QString &apkFile) = 0;

    virtual void screenshot() = 0;
    virtual void showTouch(bool show) = 0;

    virtual bool isReversePort(quint16 port) = 0;
    virtual const QString &getSerial() = 0;

    virtual void updateScript(QString script) = 0;
    virtual bool isCurrentCustomKeymap() = 0;
};

class IDeviceManage : public QObject {
    Q_OBJECT
public:
    static IDeviceManage& getInstance();

    virtual QPointer<IDevice> getDevice(const QString& serial) = 0;
    virtual bool connectDevice(DeviceParams params) = 0;
    virtual bool disconnectDevice(const QString &serial) = 0;
    virtual void disconnectAllDevice() = 0;

    virtual void updateScript(QString script) = 0;

signals:
    void deviceConnected(bool success, const QString& serial, const QString& deviceName, const QSize& size);
    void deviceDisconnected(QString serial);
};

}
