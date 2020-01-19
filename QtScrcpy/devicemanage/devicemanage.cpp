#include <QDebug>

#include "devicemanage.h"
#include "server.h"

#define DM_MAX_DEVICES_NUM 16

DeviceManage::DeviceManage(QObject *parent) : QObject(parent)
{

}

DeviceManage::~DeviceManage()
{

}

bool DeviceManage::connectDevice(Device::DeviceParams params)
{
    if (params.serial.trimmed().isEmpty()) {
        return false;
    }
    if (m_devices.contains(params.serial)) {
        return false;
    }
    if (DM_MAX_DEVICES_NUM < m_devices.size()) {
        qInfo("over the maximum number of connections");
        return false;
    }
    /*
    // 没有必要分配端口，都用27183即可，连接建立以后server会释放监听的
    quint16 port = 0;
    if (params.useReverse) {
         port = getFreePort();
        if (0 == port) {
            qInfo("no port available, automatically switch to forward");
            params.useReverse = false;
        } else {
            params.localPort = port;
            qInfo("free port %d", port);
        }
    }
    */
    Device *device = new Device(params);
    connect(device, &Device::deviceDisconnect, this, &DeviceManage::onDeviceDisconnect);
    m_devices[params.serial] = device;
    return true;
}

void DeviceManage::updateScript(QString script)
{
    QMapIterator<QString, QPointer<Device>> i(m_devices);
    while (i.hasNext()) {
        i.next();
        if (i.value()) {
            i.value()->updateScript(script);
        }
    }
}

bool DeviceManage::disconnectDevice(const QString &serial)
{
    bool ret = false;
    if (!serial.isEmpty() && m_devices.contains(serial)) {
        auto it = m_devices.find(serial);
        if (it->data()) {
            delete it->data();
            ret = true;
        }
    }
    return ret;
}

void DeviceManage::disconnectAllDevice()
{
    QMapIterator<QString, QPointer<Device>> i(m_devices);
    while (i.hasNext()) {
        i.next();
        if (i.value()) {
            delete i.value();
        }
    }
}

void DeviceManage::onDeviceDisconnect(QString serial)
{
    if (!serial.isEmpty() && m_devices.contains(serial)) {
        m_devices.remove(serial);
    }
}

quint16 DeviceManage::getFreePort()
{
    quint16 port = m_localPortStart;    
    while (port < m_localPortStart + DM_MAX_DEVICES_NUM) {
        bool used = false;
        QMapIterator<QString, QPointer<Device>> i(m_devices);
        while (i.hasNext()) {
            i.next();
            auto device = i.value();
            if (device && device->getServer()
                    && device->getServer()->isReverse()
                    && port == device->getServer()->getParams().localPort) {
                used = true;
                break;
            }
        }
        if (!used) {
            return port;
        }
        port++;
    }
    return 0;
}
