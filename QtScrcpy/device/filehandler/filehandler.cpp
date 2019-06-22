#include "filehandler.h"

#define DEVICE_SDCARD_PATH "/sdcard/"

FileHandler::FileHandler(QObject *parent)
    : QObject (parent)
{
    connect(&m_adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        switch (processResult) {
        case AdbProcess::AER_ERROR_START:
        case AdbProcess::AER_ERROR_EXEC:
        case AdbProcess::AER_ERROR_MISSING_BINARY:
            emit fileHandlerResult(FAR_ERROR_EXEC, m_isApk);
            break;
        case AdbProcess::AER_SUCCESS_EXEC:
            emit fileHandlerResult(FAR_SUCCESS_EXEC, m_isApk);
            break;
        default:
            break;
        }
    });
}

FileHandler::~FileHandler()
{

}

void FileHandler::pushFileRequest(const QString &serial, const QString &file, const QString& devicePath)
{
    if (m_adb.isRuning()) {
        emit fileHandlerResult(FAR_IS_RUNNING, false);
        return;
    }
    m_devicePath = devicePath;
    if (m_devicePath.isEmpty()) {
        m_devicePath = DEVICE_SDCARD_PATH;
    }
    m_isApk = false;
    m_adb.push(serial, file, m_devicePath);
}

void FileHandler::installApkRequest(const QString &serial, const QString &apkFile)
{
    if (m_adb.isRuning()) {
        emit fileHandlerResult(FAR_IS_RUNNING, true);
        return;
    }
    m_devicePath = "";
    m_isApk = true;
    m_adb.install(serial, apkFile);
}

const QString &FileHandler::getDevicePath()
{
    return m_devicePath;
}
