#include "filehandler.h"

FileHandler::FileHandler(QObject *parent) : QObject(parent)
{
    connect(&m_adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult) {
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

FileHandler::~FileHandler() {}

void FileHandler::onPushFileRequest(const QString &serial, const QString &file, const QString &devicePath)
{
    if (m_adb.isRuning()) {
        emit fileHandlerResult(FAR_IS_RUNNING, false);
        return;
    }

    m_isApk = false;
    m_adb.push(serial, file, devicePath);
}

void FileHandler::onInstallApkRequest(const QString &serial, const QString &apkFile)
{
    if (m_adb.isRuning()) {
        emit fileHandlerResult(FAR_IS_RUNNING, true);
        return;
    }
    m_isApk = true;
    m_adb.install(serial, apkFile);
}
