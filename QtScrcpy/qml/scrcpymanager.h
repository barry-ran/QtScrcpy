#pragma once

#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QJSEngine>
#include <QStringList>
#include <qtimer.h>
#include "adbprocess.h"
#include "scrcpyitem.h"

class ScrcpyManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringList maxSizeArray READ maxSizeArray CONSTANT FINAL)
    Q_PROPERTY(QStringList devicesList READ devicesList NOTIFY devicesListChanged)

    Q_PROPERTY(ScrcpyItem* currentDevice READ currentDevice NOTIFY currentDeviceChanged)

    Q_PROPERTY(bool currentlyConnecting READ currentlyConnecting WRITE setCurrentlyConnecting NOTIFY currentlyConnectingChanged)

    Q_PROPERTY(quint32 bitRateNumeric READ bitRateNumeric WRITE setBitRateNumeric NOTIFY bitRateNumericChanged)
    Q_PROPERTY(QString bitRateUnits READ bitRateUnits WRITE setBitRateUnits NOTIFY bitRateUnitsChanged)

    Q_PROPERTY(int maxSizeIndex READ maxSizeIndex WRITE setMaxSizeIndex NOTIFY maxSizeIndexChanged)
    Q_PROPERTY(int recordFormatIndex READ recordFormatIndex WRITE setRecordFormatIndex NOTIFY recordFormatIndexChanged)
    Q_PROPERTY(QString recordPath READ recordPath WRITE setRecordPath NOTIFY recordPathChanged)
    Q_PROPERTY(int lockOrientationIndex READ lockOrientationIndex WRITE setLockOrientationIndex NOTIFY lockOrientationIndexChanged)

    Q_PROPERTY(bool framelessWindow READ framelessWindow WRITE setFramelessWindow NOTIFY framelessWindowChanged)
    Q_PROPERTY(bool recordScreen READ recordScreen WRITE setRecordScreen NOTIFY recordScreenChanged)
    Q_PROPERTY(bool recordBackground READ recordBackground WRITE setRecordBackground NOTIFY recordBackgroundChanged)
    Q_PROPERTY(bool reverseConnect READ reverseConnect WRITE setReverseConnect NOTIFY reverseConnectChanged)
    Q_PROPERTY(bool showFPS READ showFPS WRITE setShowFPS NOTIFY showFPSChanged)
    Q_PROPERTY(bool windowOnTop READ windowOnTop WRITE setWindowOnTop NOTIFY windowOnTopChanged)
    Q_PROPERTY(bool autoOffScreen READ autoOffScreen WRITE setAutoOffScreen NOTIFY autoOffScreenChanged)
    Q_PROPERTY(bool keepAlive READ keepAlive WRITE setKeepAlive NOTIFY keepAliveChanged)
    Q_PROPERTY(bool simpleMode READ simpleMode WRITE setSimpleMode NOTIFY simpleModeChanged)
    Q_PROPERTY(bool autoUpdateDevice READ autoUpdateDevice WRITE setAutoUpdateDevice NOTIFY autoUpdateDeviceChanged)
    Q_PROPERTY(bool showToolbar READ showToolbar WRITE setShowToolbar NOTIFY showToolbarChanged)
    Q_PROPERTY(QStringList availableBitRatesUnits READ availableBitRatesUnits CONSTANT FINAL)
    Q_PROPERTY(QStringList availableOrientations READ availableOrientations CONSTANT FINAL)

public:
    explicit ScrcpyManager(QObject* parent = nullptr);
    virtual ~ScrcpyManager();

    QStringList devicesList();
    ScrcpyItem* currentDevice() const;

    bool currentlyConnecting() const;
    quint32 bitRateNumeric() const;
    QString bitRateUnits() const;

    int maxSizeIndex() const;
    int recordFormatIndex() const;
    QString recordPath() const;
    int lockOrientationIndex() const;

    bool framelessWindow() const;
    bool recordScreen() const;
    bool recordBackground() const;
    bool reverseConnect() const;
    bool showFPS() const;
    bool windowOnTop() const;
    bool autoOffScreen() const;
    bool keepAlive() const;
    bool simpleMode() const;
    bool autoUpdateDevice() const;
    bool showToolbar() const;

    QStringList maxSizeArray() const;
    QStringList availableBitRatesUnits() const;
    QStringList availableOrientations() const;

public slots:
    void listDevices();
    void connectToDevice(ScrcpyItem* item, const QString &serial);

    void disconnectFromDevice(const QString &serial);
    void disconnectFromDevice();

    void setCurrentlyConnecting(bool val);
    void setBitRateNumeric(quint32 val);
    void setBitRateUnits(const QString &val);

    void setMaxSizeIndex(int val);
    void setRecordFormatIndex(int val);
    void setRecordPath(const QString &val);
    void setLockOrientationIndex(int val);

    void setFramelessWindow(bool val);
    void setRecordScreen(bool val);
    void setRecordBackground(bool val);
    void setReverseConnect(bool val);
    void setShowFPS(bool val);
    void setWindowOnTop(bool val);
    void setAutoOffScreen(bool val);
    void setKeepAlive(bool val);
    void setSimpleMode(bool val);
    void setAutoUpdateDevice(bool val);
    void setShowToolbar(bool val);

signals:
    void devicesListChanged();
    void currentDeviceChanged();
    void currentlyConnectingChanged();

    void bitRateNumericChanged();
    void bitRateUnitsChanged();

    void maxSizeIndexChanged();
    void recordFormatIndexChanged();
    void recordPathChanged();
    void lockOrientationIndexChanged();

    void framelessWindowChanged();
    void recordScreenChanged();
    void recordBackgroundChanged();
    void reverseConnectChanged();
    void showFPSChanged();
    void windowOnTopChanged();
    void autoOffScreenChanged();
    void keepAliveChanged();
    void simpleModeChanged();
    void autoUpdateDeviceChanged();
    void showToolbarChanged();

    void deviceConnected(const QString &serial);
    void deviceDisconnected(const QString &serial);

private:
    void handleAdbResult(qsc::AdbProcess::ADB_EXEC_RESULT processResult);

    void onDeviceConnected(bool success, const QString &serial, const QString &deviceName, const QSize &size);
    void onDeviceDisconnected(const QString &serial);
    bool isAdbBusy();

    const QString &getServerPath();
    void updateBootConfig(bool toView);

private:
    qsc::AdbProcess m_adb;

    QPointer<ScrcpyItem> m_currentItem;
    QList<QString> m_deviceList;

    bool m_currentlyConnecting = false;

    quint32 m_bitRateNumeric = 0;
    QString m_bitRateUnits;

    int m_maxSizeIndex = 0;
    int m_recordFormatIndex = 0;
    QString m_recordPath;
    int m_lockOrientationIndex = 0;

    bool m_framelessWindow = false;
    bool m_recordScreen = false;
    bool m_recordBackground = false;
    bool m_reverseConnect = false;
    bool m_showFPS = false;
    bool m_windowOnTop = false;
    bool m_autoOffScreen = false;
    bool m_keepAlive = false;
    bool m_simpleMode = false;
    bool m_autoUpdateDevice = false;
    bool m_showToolbar = false;

    const QStringList m_maxSizeArray {"640", "720", "1080", "1280", "1920", "original"};
    const QStringList m_availableBitRatesUnits = {"Mbps", "Kbps"};
    const QStringList m_availableOrientations = {"no lock", "0", "90", "180", "270"};

    QTimer m_autoUpdateTimer;
};
