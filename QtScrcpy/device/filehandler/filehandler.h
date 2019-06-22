#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include <QObject>

#include "adbprocess.h"

class FileHandler : public QObject
{
    Q_OBJECT
public:
    enum FILE_HANDLER_RESULT {
        FAR_IS_RUNNING,          // 正在执行
        FAR_SUCCESS_EXEC,        // 执行成功
        FAR_ERROR_EXEC,          // 执行失败
    };

    FileHandler(QObject *parent = nullptr);
    virtual ~FileHandler();

    void pushFileRequest(const QString& serial, const QString& file, const QString& devicePath = "");
    void installApkRequest(const QString& serial, const QString& apkFile);
    const QString &getDevicePath();

signals:
    void fileHandlerResult(FILE_HANDLER_RESULT processResult, bool isApk = false);

private:
    AdbProcess m_adb;
    bool m_isApk = false;
    QString m_devicePath = "";
};

#endif // FILEHANDLER_H
