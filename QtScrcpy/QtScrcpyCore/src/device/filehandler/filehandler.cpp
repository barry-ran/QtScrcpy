#include "filehandler.h"

FileHandler::FileHandler(QObject *parent) : QObject(parent)
{
}

FileHandler::~FileHandler() {}

void FileHandler::onPushFileRequest(const QString &serial, const QString &file, const QString &devicePath)
{
    AdbProcess* adb = new AdbProcess;
    bool isApk = false;
    connect(adb, &AdbProcess::adbProcessResult, this, [this, adb, isApk](AdbProcess::ADB_EXEC_RESULT processResult) {
        onAdbProcessResult(adb, isApk, processResult);
    });

    adb->push(serial, file, devicePath);
}

void FileHandler::onInstallApkRequest(const QString &serial, const QString &apkFile)
{
    AdbProcess* adb = new AdbProcess;
    bool isApk = true;
    connect(adb, &AdbProcess::adbProcessResult, this, [this, adb, isApk](AdbProcess::ADB_EXEC_RESULT processResult) {
        onAdbProcessResult(adb, isApk, processResult);
    });

    adb->install(serial, apkFile);
}

void FileHandler::onAdbProcessResult(AdbProcess *adb, bool isApk, AdbProcess::ADB_EXEC_RESULT processResult)
{
    switch (processResult) {
    case AdbProcess::AER_ERROR_START:
    case AdbProcess::AER_ERROR_EXEC:
    case AdbProcess::AER_ERROR_MISSING_BINARY:
        emit fileHandlerResult(FAR_ERROR_EXEC, isApk);
        adb->deleteLater();
        break;
    case AdbProcess::AER_SUCCESS_EXEC:
        emit fileHandlerResult(FAR_SUCCESS_EXEC, isApk);
        adb->deleteLater();
        break;
    default:
        break;
    }
}
