#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QPointer>

class QSettings;
class Config : public QObject
{
    Q_OBJECT
public:
    static Config& getInstance();
    QString getTitle();
    QString getRecordPath();
    void setRecordPath(const QString& path);
    QString getServerVersion();
    int getMaxFps();
    int getDesktopOpenGL();
    int getSkin();
    int getRenderExpiredFrames();
    QString getPushFilePath();
    QString getServerPath();

private:
    explicit Config(QObject *parent = nullptr);
    const QString& getConfigPath();

private:
    static QString s_configPath;
    QPointer<QSettings> m_settings;
    QPointer<QSettings> m_userData;
};

#endif // CONFIG_H
