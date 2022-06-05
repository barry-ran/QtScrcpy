#ifndef DEVICEMANAGE_H
#define DEVICEMANAGE_H

#include <QMap>
#include <QPointer>

#include "../../include/QtScrcpyCore.h"

namespace qsc {

class DeviceManage : public IDeviceManage
{
    Q_OBJECT
public:
    explicit DeviceManage();
    virtual ~DeviceManage();

    virtual QPointer<IDevice> getDevice(const QString& serial) override;

    bool connectDevice(qsc::DeviceParams params) override;
    bool disconnectDevice(const QString &serial) override;
    void disconnectAllDevice() override;

protected slots:
    void onDeviceConnected(bool success, const QString& serial, const QString& deviceName, const QSize& size);
    void onDeviceDisconnected(QString serial);

private:
    quint16 getFreePort();
    void removeDevice(const QString& serial);

private:
    QMap<QString, QPointer<IDevice>> m_devices;
    quint16 m_localPortStart = 27183;
    QString m_script;
};

}
#endif // DEVICEMANAGE_H
