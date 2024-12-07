#include <QPointer>

#include "groupcontroller.h"
#include "videoform.h"

GroupController::GroupController(QObject *parent) : QObject(parent)
{

}

bool GroupController::isHost(const QString &serial)
{
    auto data = qsc::IDeviceManage::getInstance().getDevice(serial)->getUserData();
    if (!data) {
        return true;
    }

    return static_cast<VideoForm*>(data)->isHost();
}

QSize GroupController::getFrameSize(const QString &serial)
{
    auto data = qsc::IDeviceManage::getInstance().getDevice(serial)->getUserData();
    if (!data) {
        return QSize();
    }

    return static_cast<VideoForm*>(data)->frameSize();
}

GroupController &GroupController::instance()
{
    static GroupController gc;
    return gc;
}

void GroupController::updateDeviceState(const QString &serial)
{
    if (!m_devices.contains(serial)) {
        return;
    }

    auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
    if (!device) {
        return;
    }

    if (isHost(serial)) {
        device->registerDeviceObserver(this);
    } else {
        device->deRegisterDeviceObserver(this);
    }
}

void GroupController::addDevice(const QString &serial)
{
    if (m_devices.contains(serial)) {
        return;
    }

    m_devices.append(serial);
}

void GroupController::removeDevice(const QString &serial)
{
    if (!m_devices.contains(serial)) {
        return;
    }

    m_devices.removeOne(serial);

    auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
    if (!device) {
        return;
    }

    if (isHost(serial)) {
        device->deRegisterDeviceObserver(this);
    }
}

void GroupController::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    Q_UNUSED(frameSize);
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->mouseEvent(from, getFrameSize(serial), showSize);
    }
}

void GroupController::wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
    Q_UNUSED(frameSize);
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->wheelEvent(from, getFrameSize(serial), showSize);
    }
}

void GroupController::keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize)
{
    Q_UNUSED(frameSize);
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->keyEvent(from, getFrameSize(serial), showSize);
    }
}

void GroupController::postGoBack()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postGoBack();
    }
}

void GroupController::postGoHome()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postGoHome();
    }
}

void GroupController::postGoMenu()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postGoMenu();
    }
}

void GroupController::postAppSwitch()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postAppSwitch();
    }
}

void GroupController::postPower()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postPower();
    }
}

void GroupController::postVolumeUp()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postVolumeUp();
    }
}

void GroupController::postVolumeDown()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postVolumeDown();
    }
}

void GroupController::postCopy()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postCopy();
    }
}

void GroupController::postCut()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postCut();
    }
}

void GroupController::setDisplayPower(bool on)
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->setDisplayPower(on);
    }
}

void GroupController::expandNotificationPanel()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->expandNotificationPanel();
    }
}

void GroupController::collapsePanel()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->collapsePanel();
    }
}

void GroupController::postBackOrScreenOn(bool down)
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postBackOrScreenOn(down);
    }
}

void GroupController::postTextInput(QString &text)
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->postTextInput(text);
    }
}

void GroupController::requestDeviceClipboard()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->requestDeviceClipboard();
    }
}

void GroupController::setDeviceClipboard(bool pause)
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->setDeviceClipboard(pause);
    }
}

void GroupController::clipboardPaste()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->clipboardPaste();
    }
}

void GroupController::pushFileRequest(const QString &file, const QString &devicePath)
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->pushFileRequest(file, devicePath);
    }
}

void GroupController::installApkRequest(const QString &apkFile)
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->installApkRequest(apkFile);
    }
}

void GroupController::screenshot()
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->screenshot();
    }
}

void GroupController::showTouch(bool show)
{
    for (const auto& serial : m_devices) {
        if (true == isHost(serial)) {
            continue;
        }
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }

        device->showTouch(show);
    }
}
