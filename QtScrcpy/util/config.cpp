#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>

#include "config.h"

#define GROUP_COMMON "common"

#define COMMON_TITLE_KEY "WindowTitle"
#define COMMON_TITLE_DEF QCoreApplication::applicationName()

#define COMMON_RECORD_KEY "RecordPath"
#define COMMON_RECORD_DEF ""

#define COMMON_SERVER_VERSION_KEY "ServerVersion"
#define COMMON_SERVER_VERSION_DEF "1.12.1"

#define COMMON_MAX_FPS_KEY "MaxFps"
#define COMMON_MAX_FPS_DEF 60

#define COMMON_DESKTOP_OPENGL_KEY "UseDesktopOpenGL"
#define COMMON_DESKTOP_OPENGL_DEF -1

#define COMMON_SKIN_KEY "UseSkin"
#define COMMON_SKIN_DEF 1

QString Config::s_configPath = "";

Config::Config(QObject *parent) : QObject(parent)
{
    m_settings = new QSettings(getConfigPath(), QSettings::IniFormat);
    m_settings->setIniCodec("UTF-8");
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
        if (s_configPath.isEmpty() || !fileInfo.isFile()) {
            s_configPath = QCoreApplication::applicationDirPath() + "/config/config.ini";
        }
    }
    return s_configPath;
}

QString Config::getRecordPath()
{
    QString record;
    m_settings->beginGroup(GROUP_COMMON);
    record = m_settings->value(COMMON_RECORD_KEY, COMMON_RECORD_DEF).toString();
    m_settings->endGroup();
    return record;
}

void Config::setRecordPath(const QString &path)
{
    m_settings->beginGroup(GROUP_COMMON);
    m_settings->setValue(COMMON_RECORD_KEY, path);
    m_settings->endGroup();
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

QString Config::getTitle()
{
    QString title;
    m_settings->beginGroup(GROUP_COMMON);
    title = m_settings->value(COMMON_TITLE_KEY, COMMON_TITLE_DEF).toString();
    m_settings->endGroup();
    return title;
}



