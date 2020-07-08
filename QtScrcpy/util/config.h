#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QPointer>
#include <QRect>

class QSettings;
class Config : public QObject
{
    Q_OBJECT
public:
    static Config &getInstance();
    // config
    QString getTitle();
    QString getServerVersion();
    int getMaxFps();
    int getDesktopOpenGL();
    int getSkin();
    int getRenderExpiredFrames();
    QString getPushFilePath();
    QString getServerPath();
    QString getAdbPath();
    QString getLogLevel();
    QString getCodecOptions();

    // user data
    QString getRecordPath();
    void setRecordPath(const QString &path);
    int getBitRateIndex();
    void setBitRateIndex(int bitRateIndex);
    int getMaxSizeIndex();
    void setMaxSizeIndex(int maxSizeIndex);
    int getRecordFormatIndex();
    void setRecordFormatIndex(int recordFormatIndex);
    void setRect(const QString &serial, const QRect &rc);
    QRect getRect(const QString &serial);
    bool getFramelessWindow();
    void setFramelessWindow(bool frameless);

private:
    explicit Config(QObject *parent = nullptr);
    const QString &getConfigPath();

private:
    static QString s_configPath;
    QPointer<QSettings> m_settings;
    QPointer<QSettings> m_userData;
};

#endif // CONFIG_H
