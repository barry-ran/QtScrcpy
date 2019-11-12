#ifndef DEVICEMANAGE_H
#define DEVICEMANAGE_H

#include <QPointer>
#include <QMap>

#include "device.h"

class DeviceManage : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManage(QObject *parent = nullptr);
    virtual ~DeviceManage();

    bool connectDevice(Device::DeviceParams params);
    void updateScript(QString script);

    bool disconnectDevice(const QString &serial);
    void disconnectAllDevice();

protected slots:
    void onDeviceDisconnect(QString serial);

private:
    quint16 getFreePort();

private:
    QMap<QString, QPointer<Device>> m_devices;
    quint16 m_localPortStart = 27183;
};

#endif // DEVICEMANAGE_H
