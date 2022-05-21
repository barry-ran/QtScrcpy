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
    bool disconnectDevice(const QString &serial);
    void disconnectAllDevice();

    void updateScript(QString script);
    bool staysOnTop(const QString &serial);
    void showFPS(const QString &serial, bool show);

signals:
    void deviceConnected(bool success, const QString& serial, const QString& deviceName, const QSize& size);
    void deviceDisconnected(QString serial);

protected slots:
    void onDeviceConnected(bool success, const QString& serial, const QString& deviceName, const QSize& size);
    void onDeviceDisconnected(QString serial);

private:
    quint16 getFreePort();
    void removeDevice(const QString& serial);

private:
    QMap<QString, QPointer<Device>> m_devices;
    quint16 m_localPortStart = 27183;
    QString m_script;
};

#endif // DEVICEMANAGE_H
