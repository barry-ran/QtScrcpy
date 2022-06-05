#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include "adbprocess.h"
#include "adbprocessimpl.h"

QString g_adbPath;

namespace qsc {

AdbProcess::AdbProcess(QObject *parent)
    : QObject(parent)
    , m_adbImpl(new AdbProcessImpl())
{
    connect(m_adbImpl, &AdbProcessImpl::adbProcessImplResult, this, &qsc::AdbProcess::adbProcessResult);
}

AdbProcess::~AdbProcess()
{
    delete m_adbImpl;
}

void AdbProcess::setAdbPath(const QString &adbPath)
{
    g_adbPath = adbPath;
}

void AdbProcess::execute(const QString &serial, const QStringList &args)
{
    m_adbImpl->execute(serial, args);
}

bool AdbProcess::isRuning()
{
    return m_adbImpl->isRuning();
}

void AdbProcess::setShowTouchesEnabled(const QString &serial, bool enabled)
{
    m_adbImpl->setShowTouchesEnabled(serial, enabled);
}

void AdbProcess::kill()
{
    m_adbImpl->kill();
}

QStringList AdbProcess::arguments()
{
    return m_adbImpl->arguments();
}

QStringList AdbProcess::getDevicesSerialFromStdOut()
{
    return m_adbImpl->getDevicesSerialFromStdOut();
}

QString AdbProcess::getDeviceIPFromStdOut()
{
    return m_adbImpl->getDeviceIPFromStdOut();
}

QString AdbProcess::getDeviceIPByIpFromStdOut()
{
    return m_adbImpl->getDeviceIPByIpFromStdOut();
}

QString AdbProcess::getStdOut()
{
    return m_adbImpl->getStdOut();
}

QString AdbProcess::getErrorOut()
{
    return m_adbImpl->getErrorOut();
}

void AdbProcess::forward(const QString &serial, quint16 localPort, const QString &deviceSocketName)
{
    m_adbImpl->forward(serial, localPort, deviceSocketName);
}

void AdbProcess::forwardRemove(const QString &serial, quint16 localPort)
{
    m_adbImpl->forwardRemove(serial, localPort);
}

void AdbProcess::reverse(const QString &serial, const QString &deviceSocketName, quint16 localPort)
{
    m_adbImpl->reverse(serial, deviceSocketName, localPort);
}

void AdbProcess::reverseRemove(const QString &serial, const QString &deviceSocketName)
{
    m_adbImpl->reverseRemove(serial, deviceSocketName);
}

void AdbProcess::push(const QString &serial, const QString &local, const QString &remote)
{
    m_adbImpl->push(serial, local, remote);
}

void AdbProcess::install(const QString &serial, const QString &local)
{
    m_adbImpl->install(serial, local);
}

void AdbProcess::removePath(const QString &serial, const QString &path)
{
    m_adbImpl->removePath(serial, path);
}

}
