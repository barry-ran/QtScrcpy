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
            emit fileHandlerResult(FAR_ERROR_EXEC);
            break;
        case AdbProcess::AER_SUCCESS_EXEC:
            emit fileHandlerResult(FAR_SUCCESS_EXEC);
            break;
        default:
            break;
        }
    });
}

FileHandler::~FileHandler()
{

}

void FileHandler::pushFileRequest(const QString &serial, const QString &file)
{
    if (m_adb.isRuning()) {
        emit fileHandlerResult(FAR_IS_RUNNING);
        return;
    }
    m_adb.push(serial, file, DEVICE_SDCARD_PATH);
}

void FileHandler::installApkRequest(const QString &serial, const QString &apkFile)
{
    if (m_adb.isRuning()) {
        emit fileHandlerResult(FAR_IS_RUNNING);
        return;
    }
    m_adb.install(serial, apkFile);
}
