#ifndef GROUPCONTROLLER_H
#define GROUPCONTROLLER_H

#include <QObject>
#include <QVector>

#include "QtScrcpyCore.h"

class GroupController : public QObject, public qsc::DeviceObserver
{
    Q_OBJECT
public:
    static GroupController& instance();

    void updateDeviceState(const QString& serial);
    void addDevice(const QString& serial);
    void removeDevice(const QString& serial);

private:
    // DeviceObserver
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
    void setDisplayPower(bool on) override;
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

private:
    explicit GroupController(QObject *parent = nullptr);
    bool isHost(const QString& serial);
    QSize getFrameSize(const QString& serial);

private:
    QVector<QString> m_devices;
};

#endif // GROUPCONTROLLER_H
