#include "scrcpymanager.h"
#include <QDebug>
#include <QStringList>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QCoreApplication>
#include "QtScrcpyCore/include/QtScrcpyCore.h"
#include "util/config.h"

ScrcpyManager::ScrcpyManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_adb, &qsc::AdbProcess::adbProcessResult,
            this, &ScrcpyManager::handleAdbResult);

    connect(&qsc::IDeviceManage::getInstance(), &qsc::IDeviceManage::deviceConnected,
            this, &ScrcpyManager::onDeviceConnected);
    connect(&qsc::IDeviceManage::getInstance(), &qsc::IDeviceManage::deviceDisconnected,
            this, &ScrcpyManager::onDeviceDisconnected);

    connect(&m_autoUpdateTimer, &QTimer::timeout, this, &ScrcpyManager::listDevices);

    m_autoUpdateTimer.setInterval(1000);
    m_autoUpdateTimer.setSingleShot(false);

    updateBootConfig(true);
}

ScrcpyManager::~ScrcpyManager()
{
    updateBootConfig(false);
}

void ScrcpyManager::updateBootConfig(bool toView)
{
    if (toView) {
        UserBootConfig config = Config::getInstance().getUserBootConfig();

        if (config.bitRate == 0) {
            setBitRateUnits("Mbps");
        } else if (config.bitRate % 1000000 == 0) {
            setBitRateNumeric(config.bitRate / 1000000);
            setBitRateUnits("Mbps");
        } else {
            setBitRateNumeric(config.bitRate / 1000);
            setBitRateUnits("Kbps");
        }

        setMaxSizeIndex(config.maxSizeIndex);
        setRecordFormatIndex(config.recordFormatIndex);
        setRecordPath(config.recordPath);
        setLockOrientationIndex(config.lockOrientationIndex);
        setFramelessWindow(config.framelessWindow);
        setRecordScreen(config.recordScreen);
        setRecordBackground(config.recordBackground);
        setReverseConnect(config.reverseConnect);
        setShowFPS(config.showFPS);
        setWindowOnTop(config.windowOnTop);
        setAutoOffScreen(config.autoOffScreen);
        setKeepAlive(config.keepAlive);
        setSimpleMode(config.simpleMode);
        setAutoUpdateDevice(config.autoUpdateDevice);
        setShowToolbar(config.showToolbar);
    } else {
        UserBootConfig config;

        config.bitRate = (bitRateUnits() == "Mbps")
                             ? bitRateNumeric() * 1000000
                             : bitRateNumeric() * 1000;

        config.maxSizeIndex = maxSizeIndex();
        config.recordFormatIndex = recordFormatIndex();
        config.recordPath = recordPath();
        config.lockOrientationIndex = lockOrientationIndex();
        config.framelessWindow = framelessWindow();
        config.recordScreen = recordScreen();
        config.recordBackground = recordBackground();
        config.reverseConnect = reverseConnect();
        config.showFPS = showFPS();
        config.windowOnTop = windowOnTop();
        config.autoOffScreen = autoOffScreen();
        config.keepAlive = keepAlive();
        config.simpleMode = simpleMode();
        config.autoUpdateDevice = autoUpdateDevice();
        config.showToolbar = showToolbar();

        Config::getInstance().setUserBootConfig(config);
    }
}

QStringList ScrcpyManager::devicesList()
{
    return m_deviceList;
}

ScrcpyItem* ScrcpyManager::currentDevice() const
{
    return m_currentItem;
}

bool ScrcpyManager::currentlyConnecting() const
{
    return m_currentlyConnecting;
}

quint32 ScrcpyManager::bitRateNumeric() const
{
    return m_bitRateNumeric;
}

QString ScrcpyManager::bitRateUnits() const
{
    return m_bitRateUnits;
}

int ScrcpyManager::maxSizeIndex() const
{
    return m_maxSizeIndex;
}

int ScrcpyManager::recordFormatIndex() const
{
    return m_recordFormatIndex;
}

QString ScrcpyManager::recordPath() const
{
    return m_recordPath;
}

int ScrcpyManager::lockOrientationIndex() const
{
    return m_lockOrientationIndex;
}

bool ScrcpyManager::framelessWindow() const
{
    return m_framelessWindow;
}

bool ScrcpyManager::recordScreen() const
{
    return m_recordScreen;
}

bool ScrcpyManager::recordBackground() const
{
    return m_recordBackground;
}

bool ScrcpyManager::reverseConnect() const
{
    return m_reverseConnect;
}

bool ScrcpyManager::showFPS() const
{
    return m_showFPS;
}

bool ScrcpyManager::windowOnTop() const
{
    return m_windowOnTop;
}

bool ScrcpyManager::autoOffScreen() const
{
    return m_autoOffScreen;
}

bool ScrcpyManager::keepAlive() const
{
    return m_keepAlive;
}

bool ScrcpyManager::simpleMode() const
{
    return m_simpleMode;
}

bool ScrcpyManager::autoUpdateDevice() const
{
    return m_autoUpdateDevice;
}

bool ScrcpyManager::showToolbar() const
{
    return m_showToolbar;
}

void ScrcpyManager::listDevices()
{
    if (isAdbBusy()) {
        qWarning() << "[ScrcpyManager] ADB is busy, cannot list devices yet.";
        return;
    }
    m_adb.execute(QString(), {"devices"});
}

void ScrcpyManager::connectToDevice(ScrcpyItem* item, const QString &serial)
{
    if (!item) {
        qWarning() << "[ScrcpyManager] Invalid ScrcpyItem pointer!";
        return;
    }
    if (m_currentItem) {
        disconnectFromDevice(m_currentItem->serial());
    }

    setCurrentlyConnecting(true);

    qsc::DeviceParams params;
    params.serial = serial;

    QString videoSizeStr;
    if (m_maxSizeIndex < m_maxSizeArray.size() - 1) {
        videoSizeStr = m_maxSizeArray.value(m_maxSizeIndex, "640");
    } else {
        videoSizeStr = "0";
    }

    quint16 videoSize = static_cast<quint16>(videoSizeStr.toUShort());
    params.maxSize = videoSize;

    if (bitRateUnits() == "Mbps") {
        params.bitRate = bitRateNumeric() * 1000000;
    } else {
        params.bitRate = bitRateNumeric() * 1000;
    }

    params.maxFps = static_cast<quint32>(Config::getInstance().getMaxFps());
    params.closeScreen = m_autoOffScreen;
    params.useReverse = m_reverseConnect;
    params.display = !m_recordBackground;
    params.renderExpiredFrames = Config::getInstance().getRenderExpiredFrames();

    if (m_lockOrientationIndex > 0) {
        params.captureOrientationLock = 1;
        params.captureOrientation = (m_lockOrientationIndex - 1) * 90;
    }

    params.stayAwake = m_keepAlive;
    params.recordFile = m_recordScreen;
    params.recordPath = m_recordPath;

    params.serverLocalPath = getServerPath();
    params.serverRemotePath = Config::getInstance().getServerPath();
    params.pushFilePath = Config::getInstance().getPushFilePath();
    params.serverVersion = Config::getInstance().getServerVersion();
    params.logLevel = Config::getInstance().getLogLevel();
    params.codecOptions = Config::getInstance().getCodecOptions();
    params.codecName = Config::getInstance().getCodecName();
    params.gameScript = QString();
    params.scid = QRandomGenerator::global()->bounded(1, 10000) & 0x7FFFFFFF;

    qsc::IDeviceManage::getInstance().connectDevice(params);
    m_currentItem = item;
    emit currentDeviceChanged();
}

void ScrcpyManager::disconnectFromDevice(const QString &serial)
{
    bool disconnected = qsc::IDeviceManage::getInstance().disconnectDevice(serial);
    if (disconnected) {
        qDebug() << "[ScrcpyManager] Disconnect requested for" << serial;
    } else {
        qWarning() << "[ScrcpyManager] No device was disconnected (maybe not found?).";
    }

    if (m_currentItem && m_currentItem->serial() == serial) {
        m_currentItem.clear();
        emit currentDeviceChanged();
    }
    setCurrentlyConnecting(false);
}

void ScrcpyManager::disconnectFromDevice()
{
    if (m_currentItem) {
        disconnectFromDevice(m_currentItem->serial());
    }
}

void ScrcpyManager::onDeviceConnected(bool success, const QString &serial,
                                      const QString &deviceName, const QSize &size)
{
    Q_UNUSED(deviceName)
    Q_UNUSED(size)

    setCurrentlyConnecting(false);

    if (!success) {
        qWarning() << "[ScrcpyManager] Device" << serial << "failed to connect.";
        return;
    }

    if (m_currentItem) {
        auto dev = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (dev) {
            dev->setUserData(static_cast<void*>(m_currentItem.data()));
            dev->registerDeviceObserver(m_currentItem.data());
            m_currentItem->setSerial(serial);
            qDebug() << "[ScrcpyManager] Device connected:" << serial;
        }
    }
    emit deviceConnected(serial);
}

void ScrcpyManager::onDeviceDisconnected(const QString &serial)
{
    qDebug() << "[ScrcpyManager] Device disconnected:" << serial;
    if (m_currentItem && m_currentItem->serial() == serial) {
        m_currentItem.clear();
        emit currentDeviceChanged();
    }
    emit deviceDisconnected(serial);
}

bool ScrcpyManager::isAdbBusy() { return m_adb.isRuning(); }

void ScrcpyManager::handleAdbResult(qsc::AdbProcess::ADB_EXEC_RESULT processResult)
{
    switch (processResult) {
    case qsc::AdbProcess::AER_ERROR_START:
    case qsc::AdbProcess::AER_ERROR_EXEC:
        qWarning() << "[ScrcpyManager] ADB command error.";
        setCurrentlyConnecting(false);
        break;
    case qsc::AdbProcess::AER_ERROR_MISSING_BINARY:
        qWarning() << "[ScrcpyManager] ADB not found!";
        setCurrentlyConnecting(false);
        break;
    case qsc::AdbProcess::AER_SUCCESS_START:
        break;
    case qsc::AdbProcess::AER_SUCCESS_EXEC:
        if (m_adb.arguments().contains("devices")) {
            m_deviceList = m_adb.getDevicesSerialFromStdOut();
            emit devicesListChanged();
        }
        break;
    }
}

const QString &ScrcpyManager::getServerPath()
{
    static QString serverPath;
    if (serverPath.isEmpty()) {
        serverPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_SERVER_PATH"));
        QFileInfo fileInfo(serverPath);
        if (serverPath.isEmpty() || !fileInfo.isFile()) {
            serverPath = QCoreApplication::applicationDirPath() + "/scrcpy-server";
        }
    }
    return serverPath;
}

QStringList ScrcpyManager::availableOrientations() const
{
    return m_availableOrientations;
}

QStringList ScrcpyManager::availableBitRatesUnits() const
{
    return m_availableBitRatesUnits;
}

QStringList ScrcpyManager::maxSizeArray() const
{
    return m_maxSizeArray;
}

void ScrcpyManager::setCurrentlyConnecting(bool val)
{
    if (m_currentlyConnecting == val)
        return;
    m_currentlyConnecting = val;
    emit currentlyConnectingChanged();
}

void ScrcpyManager::setBitRateNumeric(quint32 val)
{
    if (m_bitRateNumeric == val)
        return;
    m_bitRateNumeric = val;
    emit bitRateNumericChanged();
}

void ScrcpyManager::setBitRateUnits(const QString &val)
{
    if (m_bitRateUnits == val)
        return;
    m_bitRateUnits = val;
    emit bitRateUnitsChanged();
}

void ScrcpyManager::setMaxSizeIndex(int val)
{
    if (m_maxSizeIndex == val)
        return;
    m_maxSizeIndex = val;
    emit maxSizeIndexChanged();
}

void ScrcpyManager::setRecordFormatIndex(int val)
{
    if (m_recordFormatIndex == val)
        return;
    m_recordFormatIndex = val;
    emit recordFormatIndexChanged();
}

void ScrcpyManager::setRecordPath(const QString &val)
{
    if (m_recordPath == val)
        return;
    m_recordPath = val;
    emit recordPathChanged();
}

void ScrcpyManager::setLockOrientationIndex(int val)
{
    if (m_lockOrientationIndex == val)
        return;
    m_lockOrientationIndex = val;
    emit lockOrientationIndexChanged();
}

void ScrcpyManager::setFramelessWindow(bool val)
{
    if (m_framelessWindow == val)
        return;
    m_framelessWindow = val;
    emit framelessWindowChanged();
}

void ScrcpyManager::setRecordScreen(bool val)
{
    if (m_recordScreen == val)
        return;
    m_recordScreen = val;
    emit recordScreenChanged();
}

void ScrcpyManager::setRecordBackground(bool val)
{
    if (m_recordBackground == val)
        return;
    m_recordBackground = val;
    emit recordBackgroundChanged();
}

void ScrcpyManager::setReverseConnect(bool val)
{
    if (m_reverseConnect == val)
        return;
    m_reverseConnect = val;
    emit reverseConnectChanged();
}

void ScrcpyManager::setShowFPS(bool val)
{
    if (m_showFPS == val)
        return;
    m_showFPS = val;
    emit showFPSChanged();
}

void ScrcpyManager::setWindowOnTop(bool val)
{
    if (m_windowOnTop == val)
        return;
    m_windowOnTop = val;
    emit windowOnTopChanged();
}

void ScrcpyManager::setAutoOffScreen(bool val)
{
    if (m_autoOffScreen == val)
        return;
    m_autoOffScreen = val;
    emit autoOffScreenChanged();
}

void ScrcpyManager::setKeepAlive(bool val)
{
    if (m_keepAlive == val)
        return;
    m_keepAlive = val;
    emit keepAliveChanged();
}

void ScrcpyManager::setSimpleMode(bool val)
{
    if (m_simpleMode == val)
        return;
    m_simpleMode = val;
    emit simpleModeChanged();
}

void ScrcpyManager::setAutoUpdateDevice(bool val)
{
    if (m_autoUpdateDevice == val)
        return;
    m_autoUpdateDevice = val;
    emit autoUpdateDeviceChanged();

    if (m_autoUpdateDevice) {
        m_autoUpdateTimer.start();
    } else {
        m_autoUpdateTimer.stop();
    }
}

void ScrcpyManager::setShowToolbar(bool val)
{
    if (m_showToolbar == val)
        return;
    m_showToolbar = val;
    emit showToolbarChanged();
}
