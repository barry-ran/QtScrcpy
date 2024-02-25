#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>
#include <QDebug>

#include "config.h"
#ifdef Q_OS_OSX
#include "path.h"
#endif

#define GROUP_COMMON "common"

// config
#define COMMON_TITLE_KEY "WindowTitle"
#define COMMON_TITLE_DEF QCoreApplication::applicationName()

#define COMMON_PUSHFILE_KEY "PushFilePath"
#define COMMON_PUSHFILE_DEF "/sdcard/"

#define COMMON_SERVER_VERSION_KEY "ServerVersion"
#define COMMON_SERVER_VERSION_DEF "2.1.1"

#define COMMON_SERVER_PATH_KEY "ServerPath"
#define COMMON_SERVER_PATH_DEF "/data/local/tmp/scrcpy-server.jar"

#define COMMON_MAX_FPS_KEY "MaxFps"
#define COMMON_MAX_FPS_DEF 0

#define COMMON_DESKTOP_OPENGL_KEY "UseDesktopOpenGL"
#define COMMON_DESKTOP_OPENGL_DEF -1

#define COMMON_SKIN_KEY "UseSkin"
#define COMMON_SKIN_DEF 1

#define COMMON_RENDER_EXPIRED_FRAMES_KEY "RenderExpiredFrames"
#define COMMON_RENDER_EXPIRED_FRAMES_DEF 0

#define COMMON_ADB_PATH_KEY "AdbPath"
#define COMMON_ADB_PATH_DEF ""

#define COMMON_LOG_LEVEL_KEY "LogLevel"
#define COMMON_LOG_LEVEL_DEF "info"

#define COMMON_CODEC_OPTIONS_KEY "CodecOptions"
#define COMMON_CODEC_OPTIONS_DEF ""

#define COMMON_CODEC_NAME_KEY "CodecName"
#define COMMON_CODEC_NAME_DEF ""

// user config
#define COMMON_RECORD_KEY "RecordPath"
#define COMMON_RECORD_DEF ""

#define COMMON_BITRATE_KEY "BitRate"
#define COMMON_BITRATE_DEF 2000000

#define COMMON_MAX_SIZE_INDEX_KEY "MaxSizeIndex"
#define COMMON_MAX_SIZE_INDEX_DEF 2

#define COMMON_RECORD_FORMAT_INDEX_KEY "RecordFormatIndex"
#define COMMON_RECORD_FORMAT_INDEX_DEF 0

#define COMMON_LOCK_ORIENTATION_INDEX_KEY "LockDirectionIndex"
#define COMMON_LOCK_ORIENTATION_INDEX_DEF 0

#define COMMON_RECORD_SCREEN_KEY "RecordScreen"
#define COMMON_RECORD_SCREEN_DEF false

#define COMMON_RECORD_BACKGROUD_KEY "RecordBackGround"
#define COMMON_RECORD_BACKGROUD_DEF false

#define COMMON_REVERSE_CONNECT_KEY "ReverseConnect"
#define COMMON_REVERSE_CONNECT_DEF true

#define COMMON_SHOW_FPS_KEY "ShowFPS"
#define COMMON_SHOW_FPS_DEF false

#define COMMON_WINDOW_ON_TOP_KEY "WindowOnTop"
#define COMMON_WINDOW_ON_TOP_DEF false

#define COMMON_AUTO_OFF_SCREEN_KEY "AutoOffScreen"
#define COMMON_AUTO_OFF_SCREEN_DEF false

#define COMMON_FRAMELESS_WINDOW_KEY "FramelessWindow"
#define COMMON_FRAMELESS_WINDOW_DEF false

#define COMMON_KEEP_ALIVE_KEY "KeepAlive"
#define COMMON_KEEP_ALIVE_DEF false

#define COMMON_SIMPLE_MODE_KEY "SimpleMode"
#define COMMON_SIMPLE_MODE_DEF false

#define COMMON_AUTO_UPDATE_DEVICE_KEY "AutoUpdateDevice"
#define COMMON_AUTO_UPDATE_DEVICE_DEF true

#define COMMON_TRAY_MESSAGE_SHOWN_KEY "TrayMessageShown"
#define COMMON_TRAY_MESSAGE_SHOWN_DEF false

// device config
#define SERIAL_WINDOW_RECT_KEY_X "WindowRectX"
#define SERIAL_WINDOW_RECT_KEY_Y "WindowRectY"
#define SERIAL_WINDOW_RECT_KEY_W "WindowRectW"
#define SERIAL_WINDOW_RECT_KEY_H "WindowRectH"
#define SERIAL_WINDOW_RECT_KEY_DEF -1
#define SERIAL_NICK_NAME_KEY "NickName"
#define SERIAL_NICK_NAME_DEF "Phone"

QString Config::s_configPath = "";

Config::Config(QObject *parent) : QObject(parent)
{
    m_settings = new QSettings(getConfigPath() + "/config.ini", QSettings::IniFormat);
    m_settings->setIniCodec("UTF-8");

    m_userData = new QSettings(getConfigPath() + "/userdata.ini", QSettings::IniFormat);
    m_userData->setIniCodec("UTF-8");

    qDebug()<<m_userData->childGroups();
}

Config &Config::getInstance()
{
    static Config config;
    return config;
}

const QString &Config::getConfigPath()
{
    if (s_configPath.isEmpty()) {
        s_configPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_CONFIG_PATH"));
        QFileInfo fileInfo(s_configPath);
        if (s_configPath.isEmpty() || !fileInfo.isDir()) {
            // default application dir
            // mac系统当从finder打开app时，默认工作目录不再是可执行程序的目录了，而是"/"
            // 而Qt的获取工作目录的api都依赖QCoreApplication的初始化，所以使用mac api获取当前目录
#ifdef Q_OS_OSX
            // get */QtScrcpy.app path
            s_configPath = Path::GetCurrentPath();
            s_configPath += "/Contents/MacOS/config";
#else
            s_configPath = "config";
#endif
        }
    }
    return s_configPath;
}

void Config::setUserBootConfig(const UserBootConfig &config)
{
    m_userData->beginGroup(GROUP_COMMON);
    m_userData->setValue(COMMON_RECORD_KEY, config.recordPath);
    m_userData->setValue(COMMON_BITRATE_KEY, config.bitRate);
    m_userData->setValue(COMMON_MAX_SIZE_INDEX_KEY, config.maxSizeIndex);
    m_userData->setValue(COMMON_RECORD_FORMAT_INDEX_KEY, config.recordFormatIndex);
    m_userData->setValue(COMMON_FRAMELESS_WINDOW_KEY, config.framelessWindow);
    m_userData->setValue(COMMON_LOCK_ORIENTATION_INDEX_KEY, config.lockOrientationIndex);
    m_userData->setValue(COMMON_RECORD_SCREEN_KEY, config.recordScreen);
    m_userData->setValue(COMMON_RECORD_BACKGROUD_KEY, config.recordBackground);
    m_userData->setValue(COMMON_REVERSE_CONNECT_KEY, config.reverseConnect);
    m_userData->setValue(COMMON_SHOW_FPS_KEY, config.showFPS);
    m_userData->setValue(COMMON_WINDOW_ON_TOP_KEY, config.windowOnTop);
    m_userData->setValue(COMMON_AUTO_OFF_SCREEN_KEY, config.autoOffScreen);
    m_userData->setValue(COMMON_KEEP_ALIVE_KEY, config.keepAlive);
    m_userData->setValue(COMMON_SIMPLE_MODE_KEY, config.simpleMode);
    m_userData->setValue(COMMON_AUTO_UPDATE_DEVICE_KEY, config.autoUpdateDevice);
    m_userData->endGroup();
    m_userData->sync();
}

UserBootConfig Config::getUserBootConfig()
{
    UserBootConfig config;
    m_userData->beginGroup(GROUP_COMMON);
    config.recordPath = m_userData->value(COMMON_RECORD_KEY, COMMON_RECORD_DEF).toString();
    config.bitRate = m_userData->value(COMMON_BITRATE_KEY, COMMON_BITRATE_DEF).toUInt();
    config.maxSizeIndex = m_userData->value(COMMON_MAX_SIZE_INDEX_KEY, COMMON_MAX_SIZE_INDEX_DEF).toInt();
    config.recordFormatIndex = m_userData->value(COMMON_RECORD_FORMAT_INDEX_KEY, COMMON_RECORD_FORMAT_INDEX_DEF).toInt();
    config.lockOrientationIndex = m_userData->value(COMMON_LOCK_ORIENTATION_INDEX_KEY, COMMON_LOCK_ORIENTATION_INDEX_DEF).toInt();
    config.framelessWindow = m_userData->value(COMMON_FRAMELESS_WINDOW_KEY, COMMON_FRAMELESS_WINDOW_DEF).toBool();
    config.recordScreen = m_userData->value(COMMON_RECORD_SCREEN_KEY, COMMON_RECORD_SCREEN_DEF).toBool();
    config.recordBackground = m_userData->value(COMMON_RECORD_BACKGROUD_KEY, COMMON_RECORD_BACKGROUD_DEF).toBool();
    config.reverseConnect = m_userData->value(COMMON_REVERSE_CONNECT_KEY, COMMON_REVERSE_CONNECT_DEF).toBool();
    config.showFPS = m_userData->value(COMMON_SHOW_FPS_KEY, COMMON_SHOW_FPS_DEF).toBool();
    config.windowOnTop = m_userData->value(COMMON_WINDOW_ON_TOP_KEY, COMMON_WINDOW_ON_TOP_DEF).toBool();
    config.autoOffScreen = m_userData->value(COMMON_AUTO_OFF_SCREEN_KEY, COMMON_AUTO_OFF_SCREEN_DEF).toBool();
    config.keepAlive = m_userData->value(COMMON_KEEP_ALIVE_KEY, COMMON_KEEP_ALIVE_DEF).toBool();
    config.simpleMode = m_userData->value(COMMON_SIMPLE_MODE_KEY, COMMON_SIMPLE_MODE_DEF).toBool();
    config.autoUpdateDevice = m_userData->value(COMMON_AUTO_UPDATE_DEVICE_KEY, COMMON_AUTO_UPDATE_DEVICE_DEF).toBool();
    m_userData->endGroup();
    return config;
}

void Config::setTrayMessageShown(bool shown)
{
    m_userData->beginGroup(GROUP_COMMON);
    m_userData->setValue(COMMON_TRAY_MESSAGE_SHOWN_KEY, shown);
    m_userData->endGroup();
    m_userData->sync();
}

bool Config::getTrayMessageShown()
{
    bool shown;
    m_userData->beginGroup(GROUP_COMMON);
    shown = m_userData->value(COMMON_TRAY_MESSAGE_SHOWN_KEY, COMMON_TRAY_MESSAGE_SHOWN_DEF).toBool();
    m_userData->endGroup();
    return shown;
}

void Config::setRect(const QString &serial, const QRect &rc)
{
    m_userData->beginGroup(serial);
    m_userData->setValue(SERIAL_WINDOW_RECT_KEY_X, rc.left());
    m_userData->setValue(SERIAL_WINDOW_RECT_KEY_Y, rc.top());
    m_userData->setValue(SERIAL_WINDOW_RECT_KEY_W, rc.width());
    m_userData->setValue(SERIAL_WINDOW_RECT_KEY_H, rc.height());
    m_userData->endGroup();
    m_userData->sync();
}

QRect Config::getRect(const QString &serial)
{
    QRect rc;
    m_userData->beginGroup(serial);
    rc.setX(m_userData->value(SERIAL_WINDOW_RECT_KEY_X, SERIAL_WINDOW_RECT_KEY_DEF).toInt());
    rc.setY(m_userData->value(SERIAL_WINDOW_RECT_KEY_Y, SERIAL_WINDOW_RECT_KEY_DEF).toInt());
    rc.setWidth(m_userData->value(SERIAL_WINDOW_RECT_KEY_W, SERIAL_WINDOW_RECT_KEY_DEF).toInt());
    rc.setHeight(m_userData->value(SERIAL_WINDOW_RECT_KEY_H, SERIAL_WINDOW_RECT_KEY_DEF).toInt());
    m_userData->endGroup();
    return rc;
}

void Config::setNickName(const QString &serial, const QString &name)
{
    m_userData->beginGroup(serial);
    m_userData->setValue(SERIAL_NICK_NAME_KEY, name);
    m_userData->endGroup();
    m_userData->sync();
}

QString Config::getNickName(const QString &serial)
{
    QString name;
    m_userData->beginGroup(serial);
    name = m_userData->value(SERIAL_NICK_NAME_KEY, SERIAL_NICK_NAME_DEF).toString();
    m_userData->endGroup();
    return name;
}

QString Config::getServerVersion()
{
    QString server;
    m_settings->beginGroup(GROUP_COMMON);
    server = m_settings->value(COMMON_SERVER_VERSION_KEY, COMMON_SERVER_VERSION_DEF).toString();
    m_settings->endGroup();
    return server;
}

int Config::getMaxFps()
{
    int fps = 0;
    m_settings->beginGroup(GROUP_COMMON);
    fps = m_settings->value(COMMON_MAX_FPS_KEY, COMMON_MAX_FPS_DEF).toInt();
    m_settings->endGroup();
    return fps;
}

int Config::getDesktopOpenGL()
{
    int opengl = 0;
    m_settings->beginGroup(GROUP_COMMON);
    opengl = m_settings->value(COMMON_DESKTOP_OPENGL_KEY, COMMON_DESKTOP_OPENGL_DEF).toInt();
    m_settings->endGroup();
    return opengl;
}

int Config::getSkin()
{
    // force disable skin
    return 0;
    int skin = 1;
    m_settings->beginGroup(GROUP_COMMON);
    skin = m_settings->value(COMMON_SKIN_KEY, COMMON_SKIN_DEF).toInt();
    m_settings->endGroup();
    return skin;
}

int Config::getRenderExpiredFrames()
{
    int renderExpiredFrames = 1;
    m_settings->beginGroup(GROUP_COMMON);
    renderExpiredFrames = m_settings->value(COMMON_RENDER_EXPIRED_FRAMES_KEY, COMMON_RENDER_EXPIRED_FRAMES_DEF).toInt();
    m_settings->endGroup();
    return renderExpiredFrames;
}

QString Config::getPushFilePath()
{
    QString pushFile;
    m_settings->beginGroup(GROUP_COMMON);
    pushFile = m_settings->value(COMMON_PUSHFILE_KEY, COMMON_PUSHFILE_DEF).toString();
    m_settings->endGroup();
    return pushFile;
}

QString Config::getServerPath()
{
    QString serverPath;
    m_settings->beginGroup(GROUP_COMMON);
    serverPath = m_settings->value(COMMON_SERVER_PATH_KEY, COMMON_SERVER_PATH_DEF).toString();
    m_settings->endGroup();
    return serverPath;
}

QString Config::getAdbPath()
{
    QString adbPath;
    m_settings->beginGroup(GROUP_COMMON);
    adbPath = m_settings->value(COMMON_ADB_PATH_KEY, COMMON_ADB_PATH_DEF).toString();
    m_settings->endGroup();
    return adbPath;
}

QString Config::getLogLevel()
{
    QString logLevel;
    m_settings->beginGroup(GROUP_COMMON);
    logLevel = m_settings->value(COMMON_LOG_LEVEL_KEY, COMMON_LOG_LEVEL_DEF).toString();
    m_settings->endGroup();
    return logLevel;
}

QString Config::getCodecOptions()
{
    QString codecOptions;
    m_settings->beginGroup(GROUP_COMMON);
    codecOptions = m_settings->value(COMMON_CODEC_OPTIONS_KEY, COMMON_CODEC_OPTIONS_DEF).toString();
    m_settings->endGroup();
    return codecOptions;
}

QString Config::getCodecName()
{
    QString codecName;
    m_settings->beginGroup(GROUP_COMMON);
    codecName = m_settings->value(COMMON_CODEC_NAME_KEY, COMMON_CODEC_NAME_DEF).toString();
    m_settings->endGroup();
    return codecName;
}

QStringList Config::getConnectedGroups()
{
    return m_userData->childGroups();
}

void Config::deleteGroup(const QString &serial)
{
    m_userData->remove(serial);
}

QString Config::getTitle()
{
    QString title;
    m_settings->beginGroup(GROUP_COMMON);
    title = m_settings->value(COMMON_TITLE_KEY, COMMON_TITLE_DEF).toString();
    m_settings->endGroup();
    return title;
}
