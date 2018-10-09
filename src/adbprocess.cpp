#include "adbprocess.h"

#include <QProcess>
#include <QCoreApplication>
#include <QDebug>

QString AdbProcess::s_adbPath = "";

AdbProcess::AdbProcess(QObject *parent)
    : QProcess(parent)
{
    initSignals();
}

AdbProcess::~AdbProcess()
{
    if (isRuning()) {
        close();
    }
}

const QString& AdbProcess::getAdbPath()
{
    if (s_adbPath.isEmpty()) {
        s_adbPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_ADB_PATH"));
        if (s_adbPath.isEmpty()) {
            s_adbPath = "adb";
        }
    }
    return s_adbPath;
}

void AdbProcess::initSignals()
{
    // aboutToQuit not exit event loop, so deletelater is ok
    //connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &AdbProcess::deleteLater);

    connect(this, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
          [this](int exitCode, QProcess::ExitStatus exitStatus){
        if (NormalExit == exitStatus && 0 == exitCode) {
            emit adbProcessResult(AER_SUCCESS);
        } else {
            emit adbProcessResult(AER_ERROR_CMD);
        }

        qDebug() << ">>>>>>>>" << __FUNCTION__;
        qDebug() << "adb return " << exitCode << "exit status " << exitStatus;
    });

    connect(this, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error){
        if (QProcess::FailedToStart == error) {
            emit adbProcessResult(AER_ERROR_MISSING_BINARY);
        } else {
            emit adbProcessResult(AER_ERROR_START);
            QString err = QString("qprocess start error:%1 %2").arg(program()).arg(arguments().join(" "));
            qCritical(err.toStdString().c_str());            
        }
    });

    connect(this, &QProcess::readyReadStandardError, this,
            [this](){
        qDebug() << ">>>>>>>>" << __FUNCTION__;
        qDebug() << QString::fromLocal8Bit(readAllStandardError());
    });

    connect(this, &QProcess::readyReadStandardOutput, this,
            [this](){
        qDebug() << ">>>>>>>>" << __FUNCTION__;
        qDebug() << QString::fromLocal8Bit(readAllStandardOutput());
    });

    connect(this, &QProcess::started, this,
            [this](){

    });
}

void AdbProcess::execute(const QString& serial, const QStringList& args)
{
    QStringList adbArgs;
    if (!serial.isEmpty()) {
        adbArgs << "-s" << serial;
    }
    adbArgs << args;
    start(getAdbPath(), adbArgs);
    //start("C:\\Users\\Barry\\Desktop\\sockettool.exe", Q_NULLPTR);
}

bool AdbProcess::isRuning()
{
    if (QProcess::NotRunning == state()) {
        return false;
    } else {
        return true;
    }
}

void AdbProcess::forward(const QString& serial, quint16 localPort, const QString& deviceSocketName)
{
    QStringList adbArgs;
    adbArgs << "forward";
    adbArgs << QString("tcp:%1").arg(localPort);
    adbArgs << QString("localabstract:%1").arg(deviceSocketName);
    execute(serial, adbArgs);
}

void AdbProcess::forwardRemove(const QString& serial, quint16 localPort)
{
    QStringList adbArgs;
    adbArgs << "forward";
    adbArgs << "--remove";
    adbArgs << QString("tcp:%1").arg(localPort);
    execute(serial, adbArgs);
}

void AdbProcess::reverse(const QString& serial, const QString& deviceSocketName, quint16 localPort)
{
    QStringList adbArgs;
    adbArgs << "reverse";
    adbArgs << QString("localabstract:%1").arg(deviceSocketName);
    adbArgs << QString("tcp:%1").arg(localPort);
    execute(serial, adbArgs);
}

void AdbProcess::reverseRemove(const QString& serial, const QString& deviceSocketName)
{
    QStringList adbArgs;
    adbArgs << "reverse";
    adbArgs << "--remove";
    adbArgs << QString("localabstract:%1").arg(deviceSocketName);
    execute(serial, adbArgs);
}

void AdbProcess::push(const QString& serial, const QString& local, const QString& remote)
{
    QStringList adbArgs;
    adbArgs << "push";
    adbArgs << local;
    adbArgs << remote;
    execute(serial, adbArgs);
}

void AdbProcess::install(const QString& serial, const QString& local)
{
    QStringList adbArgs;
    adbArgs << "install";
    adbArgs << "-r";
    adbArgs << local;
    execute(serial, adbArgs);
}

void AdbProcess::removePath(const QString& serial, const QString& path)
{
    QStringList adbArgs;
    adbArgs << "shell";
    adbArgs << "rm";
    adbArgs << path;
    execute(serial, adbArgs);
}
