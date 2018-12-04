#include <QProcess>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>

#include "adbprocess.h"

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
        QFileInfo fileInfo(s_adbPath);
        if (s_adbPath.isEmpty() || !fileInfo.isFile()) {
            s_adbPath = QCoreApplication::applicationDirPath() + "/adb";
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
            emit adbProcessResult(AER_SUCCESS_EXEC);
        } else {
            //P7C0218510000537        unauthorized ,手机端此时弹出调试认证，要允许调试
            emit adbProcessResult(AER_ERROR_EXEC);
        }
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
        m_errorOutput = QString::fromLocal8Bit(readAllStandardError()).trimmed();        
        qWarning(QString("AdbProcess::error:%1").arg(m_errorOutput).toUtf8());
    });

    connect(this, &QProcess::readyReadStandardOutput, this,
            [this](){
        m_standardOutput = QString::fromLocal8Bit(readAllStandardOutput()).trimmed();
        qInfo(QString("AdbProcess::out:%1").arg(m_standardOutput).toUtf8());
    });

    connect(this, &QProcess::started, this,
            [this](){
        emit adbProcessResult(AER_SUCCESS_START);
    });
}

void AdbProcess::execute(const QString& serial, const QStringList& args)
{
    m_standardOutput = "";
    m_errorOutput = "";
    QStringList adbArgs;
    if (!serial.isEmpty()) {
        adbArgs << "-s" << serial;
    }
    adbArgs << args;
    qDebug() << getAdbPath() << adbArgs.join(" ");
    start(getAdbPath(), adbArgs);
}

bool AdbProcess::isRuning()
{
    if (QProcess::NotRunning == state()) {
        return false;
    } else {
        return true;
    }
}

void AdbProcess::setShowTouchesEnabled(const QString &serial, bool enabled)
{
    QStringList adbArgs;
    adbArgs << "shell" << "settings" << "put" << "system" << "show_touches";
    adbArgs << (enabled ? "1" : "0");
    execute(serial, adbArgs);
}

QStringList AdbProcess::getDevicesSerialFromStdOut()
{
    // get devices serial by adb devices
    QStringList serials;
    QStringList devicesInfoList = m_standardOutput.split(QRegExp("\r\n|\n"), QString::SkipEmptyParts);
    for(QString deviceInfo : devicesInfoList) {
        QStringList deviceInfos = deviceInfo.split(QRegExp("\t"), QString::SkipEmptyParts);
        if (2 == deviceInfos.count() && 0 == deviceInfos[1].compare("device")) {
            serials << deviceInfos[0];
        }
    }
    return serials;
}

QString AdbProcess::getDeviceIPFromStdOut()
{
    QString ip = "";
    QString strIPExp = "inet [\\d.]*";
    QRegExp ipRegExp(strIPExp,Qt::CaseInsensitive);
    if (ipRegExp.indexIn(m_standardOutput) != -1) {
        ip = ipRegExp.cap(0);
        ip = ip.right(ip.size() - 5);
    }
    return ip;
}

QString AdbProcess::getStdOut()
{
    return m_standardOutput;
}

QString AdbProcess::getErrorOut()
{
    return m_errorOutput;
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
