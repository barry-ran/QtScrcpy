#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>

#include "config.h"

#define GROUP_COMMON "common"

// config
#define COMMON_TITLE_KEY "WindowTitle"
#define COMMON_TITLE_DEF QCoreApplication::applicationName()

#define COMMON_PUSHFILE_KEY "PushFilePath"
#define COMMON_PUSHFILE_DEF "/sdcard/"

#define COMMON_SERVER_VERSION_KEY "ServerVersion"
#define COMMON_SERVER_VERSION_DEF "1.12.1"

#define COMMON_SERVER_PATH_KEY "ServerPath"
#define COMMON_SERVER_PATH_DEF "/data/local/tmp/scrcpy-server.jar"

#define COMMON_MAX_FPS_KEY "MaxFps"
#define COMMON_MAX_FPS_DEF 60

#define COMMON_DESKTOP_OPENGL_KEY "UseDesktopOpenGL"
#define COMMON_DESKTOP_OPENGL_DEF -1

#define COMMON_SKIN_KEY "UseSkin"
#define COMMON_SKIN_DEF 1

#define COMMON_RENDER_EXPIRED_FRAMES_KEY "RenderExpiredFrames"
#define COMMON_RENDER_EXPIRED_FRAMES_DEF 0

// user data
#define COMMON_RECORD_KEY "RecordPath"
#define COMMON_RECORD_DEF ""

#define COMMON_BITRATE_INDEX_KEY "BitRateIndex"
#define COMMON_BITRATE_INDEX_DEF 2

#define COMMON_MAX_SIZE_INDEX_KEY "MaxSizeIndex"
#define COMMON_MAX_SIZE_INDEX_DEF 2

#define COMMON_RECORD_FORMAT_INDEX_KEY "RecordFormatIndex"
#define COMMON_RECORD_FORMAT_INDEX_DEF 0

// 最大尺寸 录制格式

QString Config::s_configPath = "";

Config::Config(QObject *parent) : QObject(parent)
{
    m_settings = new QSettings(getConfigPath() + "/config.ini", QSettings::IniFormat);
    m_settings->setIniCodec("UTF-8");

    m_userData = new QSettings(getConfigPath() + "/userdata.ini", QSettings::IniFormat);
    m_userData->setIniCodec("UTF-8");
}

Config &Config::getInstance()
{
    static Config config;
    return config;
}

const QString& Config::getConfigPath()
{
    if (s_configPath.isEmpty()) {
        s_configPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_CONFIG_PATH"));
        QFileInfo fileInfo(s_configPath);
        if (s_configPath.isEmpty() || !fileInfo.isDir()) {
            // default application dir
            s_configPath = "config";
        }
    }
    return s_configPath;
}

QString Config::getRecordPath()
{
    QString record;
    m_userData->beginGroup(GROUP_COMMON);
    record = m_userData->value(COMMON_RECORD_KEY, COMMON_RECORD_DEF).toString();
    m_userData->endGroup();
    return record;
}

void Config::setRecordPath(const QString &path)
{
    m_userData->beginGroup(GROUP_COMMON);
    m_userData->setValue(COMMON_RECORD_KEY, path);
    m_userData->endGroup();
}

int Config::getBitRateIndex()
{
    int bitRateIndex;
    m_userData->beginGroup(GROUP_COMMON);
    bitRateIndex = m_userData->value(COMMON_BITRATE_INDEX_KEY, COMMON_BITRATE_INDEX_DEF).toInt();
    m_userData->endGroup();
    return bitRateIndex;
}

void Config::setBitRateIndex(int bitRateIndex)
{
    m_userData->beginGroup(GROUP_COMMON);
    m_userData->setValue(COMMON_BITRATE_INDEX_KEY, bitRateIndex);
    m_userData->endGroup();
}

int Config::getMaxSizeIndex()
{
    int maxSizeIndex;
    m_userData->beginGroup(GROUP_COMMON);
    maxSizeIndex = m_userData->value(COMMON_MAX_SIZE_INDEX_KEY, COMMON_MAX_SIZE_INDEX_DEF).toInt();
    m_userData->endGroup();
    return maxSizeIndex;
}

void Config::setMaxSizeIndex(int maxSizeIndex)
{
    m_userData->beginGroup(GROUP_COMMON);
    m_userData->setValue(COMMON_MAX_SIZE_INDEX_KEY, maxSizeIndex);
    m_userData->endGroup();
}

int Config::getRecordFormatIndex()
{
    int recordFormatIndex;
    m_userData->beginGroup(GROUP_COMMON);
    recordFormatIndex = m_userData->value(COMMON_RECORD_FORMAT_INDEX_KEY, COMMON_RECORD_FORMAT_INDEX_DEF).toInt();
    m_userData->endGroup();
    return recordFormatIndex;
}

void Config::setRecordFormatIndex(int recordFormatIndex)
{
    m_userData->beginGroup(GROUP_COMMON);
    m_userData->setValue(COMMON_RECORD_FORMAT_INDEX_KEY, recordFormatIndex);
    m_userData->endGroup();
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
    int fps = 60;
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

QString Config::getTitle()
{
    QString title;
    m_settings->beginGroup(GROUP_COMMON);
    title = m_settings->value(COMMON_TITLE_KEY, COMMON_TITLE_DEF).toString();
    m_settings->endGroup();
    return title;
}



