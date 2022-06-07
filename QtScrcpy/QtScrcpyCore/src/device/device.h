#ifndef DEVICE_H
#define DEVICE_H

#include <set>
#include <QElapsedTimer>
#include <QPointer>
#include <QTime>

#include "../../include/QtScrcpyCore.h"

#include "controlmsg.h"

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class Recorder;
class Server;
class VideoBuffer;
class Decoder;
class FileHandler;
class Stream;
class VideoForm;
class Controller;
struct AVFrame;

namespace qsc {

class Device : public IDevice
{
    Q_OBJECT
public:
    explicit Device(DeviceParams params, QObject *parent = nullptr);
    virtual ~Device();

    void setUserData(void* data) override;
    void* getUserData() override;

    void registerDeviceObserver(DeviceObserver* observer) override;
    void deRegisterDeviceObserver(DeviceObserver* observer) override;

    bool connectDevice() override;
    void disconnectDevice() override;

    // key map
    void mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize) override;
    void wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize) override;
    void keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize) override;

    void postGoBack() override;
    void postGoHome() override;
    void postGoMenu() override;
    void postAppSwitch() override;
    void postPower() override;
    void postVolumeUp() override;
    void postVolumeDown() override;
    void postCopy() override;
    void postCut() override;
    void setScreenPowerMode(bool open) override;
    void expandNotificationPanel() override;
    void collapsePanel() override;
    void postBackOrScreenOn(bool down) override;
    void postTextInput(QString &text) override;
    void requestDeviceClipboard() override;
    void setDeviceClipboard(bool pause = true) override;
    void clipboardPaste() override;
    void pushFileRequest(const QString &file, const QString &devicePath = "") override;
    void installApkRequest(const QString &apkFile) override;

    void screenshot() override;
    void showTouch(bool show) override;

    bool isReversePort(quint16 port) override;
    const QString &getSerial() override;

    void updateScript(QString script) override;
    bool isCurrentCustomKeymap() override;

private:
    void initSignals();
    bool saveFrame(int width, int height, uint8_t* dataRGB32);

private:
    // server relevant
    QPointer<Server> m_server;
    QPointer<Decoder> m_decoder;
    QPointer<Controller> m_controller;
    QPointer<FileHandler> m_fileHandler;
    QPointer<Stream> m_stream;
    QPointer<Recorder> m_recorder;

    QElapsedTimer m_startTimeCount;
    DeviceParams m_params;
    std::set<DeviceObserver*> m_deviceObservers;
    void* m_userData = nullptr;
};

}

#endif // DEVICE_H
