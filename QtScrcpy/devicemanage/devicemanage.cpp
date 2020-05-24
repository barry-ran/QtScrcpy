#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include "devicemanage.h"
#include "server.h"
#include "videoform.h"

#define DM_MAX_DEVICES_NUM 16

DeviceManage::DeviceManage(QObject *parent) : QObject(parent) {}

DeviceManage::~DeviceManage() {}

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
    connect(device, &Device::controlStateChange, this, &DeviceManage::onControlStateChange);
    m_devices[params.serial] = device;
    if (!m_script.isEmpty()) {
        device->updateScript(m_script);
    }
    return true;
}

void DeviceManage::updateScript(QString script)
{
    m_script = script;
    QMapIterator<QString, QPointer<Device>> i(m_devices);
    while (i.hasNext()) {
        i.next();
        if (i.value()) {
            i.value()->updateScript(script);
        }
    }
}

bool DeviceManage::staysOnTop(const QString &serial)
{
    if (!serial.isEmpty() && m_devices.contains(serial)) {
        auto it = m_devices.find(serial);
        if (!it->data()) {
            return false;
        }
        if (!it->data()->getVideoForm()) {
            return false;
        }
        it->data()->getVideoForm()->staysOnTop();
    }
    return true;
}

void DeviceManage::showFPS(const QString &serial, bool show)
{
    if (!serial.isEmpty() && m_devices.contains(serial)) {
        auto it = m_devices.find(serial);
        if (!it->data()) {
            return;
        }
        if (!it->data()->getVideoForm()) {
            return;
        }
        it->data()->getVideoForm()->showFPS(show);
    }
    return;
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

void DeviceManage::setGroupControlSignals(Device *host, Device *client, bool install)
{
    if (!host || !client) {
        return;
    }
    if (install) {
        connect(host, &Device::postGoBack, client, &Device::postGoBack);
        connect(host, &Device::postGoHome, client, &Device::postGoHome);
        connect(host, &Device::postGoMenu, client, &Device::postGoMenu);
        connect(host, &Device::postAppSwitch, client, &Device::postAppSwitch);
        connect(host, &Device::postPower, client, &Device::postPower);
        connect(host, &Device::postVolumeUp, client, &Device::postVolumeUp);
        connect(host, &Device::postVolumeDown, client, &Device::postVolumeDown);
        connect(host, &Device::setScreenPowerMode, client, &Device::setScreenPowerMode);
        connect(host, &Device::expandNotificationPanel, client, &Device::expandNotificationPanel);
        connect(host, &Device::collapseNotificationPanel, client, &Device::collapseNotificationPanel);
        connect(host, &Device::postBackOrScreenOn, client, &Device::postBackOrScreenOn);
        connect(host, &Device::postTextInput, client, &Device::postTextInput);
        connect(host, &Device::setDeviceClipboard, client, &Device::setDeviceClipboard);
        connect(host, &Device::clipboardPaste, client, &Device::clipboardPaste);
        connect(host, &Device::pushFileRequest, client, &Device::pushFileRequest);
        connect(host, &Device::installApkRequest, client, &Device::installApkRequest);
        connect(host, &Device::screenshot, client, &Device::screenshot);
        connect(host, &Device::showTouch, client, &Device::showTouch);
        // dont connect requestDeviceClipboard
        //connect(host, &Device::requestDeviceClipboard, client, &Device::requestDeviceClipboard);
    } else {
        disconnect(host, &Device::postGoBack, client, &Device::postGoBack);
        disconnect(host, &Device::postGoHome, client, &Device::postGoHome);
        disconnect(host, &Device::postGoMenu, client, &Device::postGoMenu);
        disconnect(host, &Device::postAppSwitch, client, &Device::postAppSwitch);
        disconnect(host, &Device::postPower, client, &Device::postPower);
        disconnect(host, &Device::postVolumeUp, client, &Device::postVolumeUp);
        disconnect(host, &Device::postVolumeDown, client, &Device::postVolumeDown);
        disconnect(host, &Device::setScreenPowerMode, client, &Device::setScreenPowerMode);
        disconnect(host, &Device::expandNotificationPanel, client, &Device::expandNotificationPanel);
        disconnect(host, &Device::collapseNotificationPanel, client, &Device::collapseNotificationPanel);
        disconnect(host, &Device::postBackOrScreenOn, client, &Device::postBackOrScreenOn);
        disconnect(host, &Device::postTextInput, client, &Device::postTextInput);
        disconnect(host, &Device::setDeviceClipboard, client, &Device::setDeviceClipboard);
        disconnect(host, &Device::clipboardPaste, client, &Device::clipboardPaste);
        disconnect(host, &Device::pushFileRequest, client, &Device::pushFileRequest);
        disconnect(host, &Device::installApkRequest, client, &Device::installApkRequest);
        disconnect(host, &Device::screenshot, client, &Device::screenshot);
        disconnect(host, &Device::showTouch, client, &Device::showTouch);
    }
}

void DeviceManage::setGroupControlHost(Device *host, bool install)
{
    QMapIterator<QString, QPointer<Device>> i(m_devices);
    while (i.hasNext()) {
        i.next();
        if (!i.value()) {
            continue;
        }
        if (i.value() == host) {
            continue;
        }
        if (install) {
            if (host) {
                setGroupControlSignals(host, i.value(), true);
            }
            emit i.value()->setControlState(i.value(), Device::GroupControlState::GCS_CLIENT);
        } else {
            if (host) {
                setGroupControlSignals(host, i.value(), false);
            }
            emit i.value()->setControlState(i.value(), Device::GroupControlState::GCS_FREE);
        }
    }
}

void DeviceManage::onDeviceDisconnect(QString serial)
{
    if (!serial.isEmpty() && m_devices.contains(serial)) {
        if (m_devices[serial]->controlState() == Device::GroupControlState::GCS_HOST) {
            setGroupControlHost(nullptr, false);
        }
        m_devices.remove(serial);
    }
}

void DeviceManage::onControlStateChange(Device *device, Device::GroupControlState oldState, Device::GroupControlState newState)
{
    if (!device) {
        return;
    }
    // free to host
    if (oldState == Device::GroupControlState::GCS_FREE && newState == Device::GroupControlState::GCS_HOST) {
        // install direct control signals
        setGroupControlHost(device, true);
        // install convert control signals(frameSize need convert)
        connect(device, &Device::mouseEvent, this, &DeviceManage::onMouseEvent, Qt::UniqueConnection);
        connect(device, &Device::wheelEvent, this, &DeviceManage::onWheelEvent, Qt::UniqueConnection);
        connect(device, &Device::keyEvent, this, &DeviceManage::onKeyEvent, Qt::UniqueConnection);
        return;
    }
    // host to free
    if (oldState == Device::GroupControlState::GCS_HOST && newState == Device::GroupControlState::GCS_FREE) {
        // uninstall direct control signals
        setGroupControlHost(device, false);
        // uninstall convert control signals(frameSize need convert)
        disconnect(device, &Device::mouseEvent, this, &DeviceManage::onMouseEvent);
        disconnect(device, &Device::wheelEvent, this, &DeviceManage::onWheelEvent);
        disconnect(device, &Device::keyEvent, this, &DeviceManage::onKeyEvent);
        return;
    }
}

void DeviceManage::onMouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    Q_UNUSED(frameSize)
    QMapIterator<QString, QPointer<Device>> i(m_devices);
    while (i.hasNext()) {
        i.next();
        if (!i.value()) {
            continue;
        }
        if (i.value() == sender()) {
            continue;
        }
        // neend convert frameSize to its frameSize
        emit i.value()->mouseEvent(from, i.value()->frameSize(), showSize);
    }
}

void DeviceManage::onWheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
    Q_UNUSED(frameSize)
    QMapIterator<QString, QPointer<Device>> i(m_devices);
    while (i.hasNext()) {
        i.next();
        if (!i.value()) {
            continue;
        }
        if (i.value() == sender()) {
            continue;
        }
        // neend convert frameSize to its frameSize
        emit i.value()->wheelEvent(from, i.value()->frameSize(), showSize);
    }
}

void DeviceManage::onKeyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize)
{
    Q_UNUSED(frameSize)
    QMapIterator<QString, QPointer<Device>> i(m_devices);
    while (i.hasNext()) {
        i.next();
        if (!i.value()) {
            continue;
        }
        if (i.value() == sender()) {
            continue;
        }
        // neend convert frameSize to its frameSize
        emit i.value()->keyEvent(from, i.value()->frameSize(), showSize);
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
            if (device && device->getServer() && device->getServer()->isReverse() && port == device->getServer()->getParams().localPort) {
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
