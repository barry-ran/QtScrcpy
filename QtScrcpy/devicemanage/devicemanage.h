#ifndef DEVICEMANAGE_H
#define DEVICEMANAGE_H

#include <QMap>
#include <QPointer>

#include "device.h"

class DeviceManage : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManage(QObject *parent = nullptr);
    virtual ~DeviceManage();

    bool connectDevice(Device::DeviceParams params);
    void updateScript(QString script);
    bool staysOnTop(const QString &serial);
    void showFPS(const QString &serial, bool show);

    bool disconnectDevice(const QString &serial);
    void disconnectAllDevice();

protected:
    void setGroupControlSignals(Device *host, Device *client, bool install);
    void setGroupControlHost(Device *host, bool install);

protected slots:
    void onDeviceDisconnect(QString serial);
    void onControlStateChange(Device *device, Device::GroupControlState oldState, Device::GroupControlState newState);

    // neend convert frameSize to its frameSize
    void onMouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize);
    void onWheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize);
    void onKeyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize);

private:
    quint16 getFreePort();

private:
    QMap<QString, QPointer<Device>> m_devices;
    quint16 m_localPortStart = 27183;
    QString m_script;
};

#endif // DEVICEMANAGE_H
