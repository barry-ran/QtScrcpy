#include <QDebug>

#include "server.h"

#define DEVICE_SERVER_PATH "/data/local/tmp/scrcpy-server.jar"
#define SOCKET_NAME "qtscrcpy"

Server::Server(QObject *parent) : QObject(parent)
{
    connect(&m_workProcess, &AdbProcess::adbProcessResult, this, &Server::onWorkProcessResult);
}

const QString& Server::getServerPath()
{
    if (m_serverPath.isEmpty()) {
        m_serverPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_SERVER_PATH"));
        if (m_serverPath.isEmpty()) {
            m_serverPath = "scrcpy-server.jar";
        }
    }
    return m_serverPath;
}

bool Server::pushServer()
{    
    if (m_workProcess.isRuning()) {
        m_workProcess.kill();
    }
    m_workProcess.push(m_serial, getServerPath(), DEVICE_SERVER_PATH);
    return true;
}

bool Server::removeServer()
{
    if (m_workProcess.isRuning()) {
        m_workProcess.kill();
    }
    m_workProcess.removePath(m_serial, DEVICE_SERVER_PATH);
    return true;
}

bool Server::enableTunnelReverse()
{
    if (m_workProcess.isRuning()) {
        m_workProcess.kill();
    }
    m_workProcess.reverse(m_serial, SOCKET_NAME, m_localPort);
    return true;
}

bool Server::disableTunnelReverse()
{
    if (m_workProcess.isRuning()) {
        m_workProcess.kill();
    }
    m_workProcess.reverseRemove(m_serial, SOCKET_NAME);
    return true;
}

bool Server::enableTunnelForward()
{
    if (m_workProcess.isRuning()) {
        m_workProcess.kill();
    }
    m_workProcess.forward(m_serial, m_localPort, SOCKET_NAME);
    return true;
}
bool Server::disableTunnelForward()
{
    if (m_workProcess.isRuning()) {
        m_workProcess.kill();
    }
    m_workProcess.forwardRemove(m_serial, m_localPort);
    return true;
}

bool Server::execute()
{
    AdbProcess* adb = new AdbProcess();
    if (!adb) {
        return false;
    }
    QStringList args;
    args << "shell";
    args << QString("CLASSPATH=%1").arg(DEVICE_SERVER_PATH);
    args << "app_process";
    args << "/"; // unused;
    args << "com.genymobile.scrcpy.Server";
    args << QString::number(m_maxSize);
    args << QString::number(m_bitRate);
    args << (m_tunnelForward ? "true" : "false");
    args << (m_crop.isEmpty() ? "" : m_crop);

    connect(adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        if (AdbProcess::AER_SUCCESS == processResult) {

        }
        sender()->deleteLater();
    });
    adb->execute(m_serial, args);
    return true;
}

bool Server::start(const QString& serial, quint16 localPort, quint16 maxSize, quint32 bitRate, const QString& crop)
{
    if (serial.isEmpty()) {
        return false;
    }

    m_localPort = localPort;
    m_maxSize = maxSize;
    m_bitRate = bitRate;
    m_crop = crop;

    m_serverStartStep = SSS_PUSH;
    return startServerByStep();
}

bool Server::startServerByStep()
{
    // push, enable tunnel et start the server
    if (SSS_NULL != m_serverStartStep) {
        switch (m_serverStartStep) {
        case SSS_PUSH:
            return pushServer();
        case SSS_ENABLE_TUNNEL_REVERSE:
            return enableTunnelReverse();
            break;
        case SSS_ENABLE_TUNNEL_FORWARD:
            return enableTunnelForward();
            break;
        case SSS_EXECUTE_SERVER:
            // if "adb reverse" does not work (e.g. over "adb connect"), it fallbacks to
            // "adb forward", so the app socket is the client
            if (m_tunnelForward) {
                // At the application level, the device part is "the server" because it
                // serves video stream and control. However, at the network level, the
                // client listens and the server connects to the client. That way, the
                // client can listen before starting the server app, so there is no need to
                // try to connect until the server socket is listening on the device.
                m_serverSocket.setMaxPendingConnections(1);
                if (!m_serverSocket.listen(QHostAddress::LocalHost, m_localPort)) {
                    qCritical(QString("Could not listen on port %1").arg(m_localPort).toStdString().c_str());
                    m_serverStartStep = SSS_NULL;
                    if (m_tunnelForward) {
                        disableTunnelForward();
                    } else {
                        disableTunnelReverse();
                    }
                    emit serverStartResult(false);
                    return false;
                }
            }
            // server will connect to our server socket
            return execute();
            break;
        default:
            break;
        }
    }
    return true;
}

void Server::onWorkProcessResult(AdbProcess::ADB_EXEC_RESULT processResult)
{
    if (SSS_NULL != m_serverStartStep) {
        switch (m_serverStartStep) {
        case SSS_PUSH:
            if (AdbProcess::AER_SUCCESS != processResult) {
                qCritical("adb push");
                m_serverStartStep = SSS_NULL;
                emit serverStartResult(false);
            } else {
                m_serverCopiedToDevice = true;
                m_serverStartStep = SSS_ENABLE_TUNNEL_REVERSE;
                startServerByStep();
            }
            break;
        case SSS_ENABLE_TUNNEL_REVERSE:
            if (AdbProcess::AER_SUCCESS != processResult) {
                qCritical("adb reverse");
                m_tunnelForward = true;
                m_serverStartStep = SSS_ENABLE_TUNNEL_FORWARD;
                startServerByStep();
            } else {
                m_serverStartStep = SSS_EXECUTE_SERVER;
                startServerByStep();
            }
            break;
        case SSS_ENABLE_TUNNEL_FORWARD:
            if (AdbProcess::AER_SUCCESS != processResult) {
                qCritical("adb forward");
                m_serverStartStep = SSS_NULL;
                emit serverStartResult(false);
            } else {
                m_serverStartStep = SSS_EXECUTE_SERVER;
                startServerByStep();
            }
            break;
        case SSS_EXECUTE_SERVER:
            if (AdbProcess::AER_SUCCESS != processResult) {
                if (!m_tunnelForward) {
                    m_serverSocket.close();
                    disableTunnelReverse();
                } else {
                    disableTunnelForward();
                }
                qCritical("adb shell start server failed");
                m_serverStartStep = SSS_NULL;
                emit serverStartResult(false);
            } else {
                m_serverStartStep = SSS_NULL;
                m_tunnelEnabled = true;
                emit serverStartResult(true);
            }
            break;
        default:
            break;
        }
    }
}
