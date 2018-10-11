#include <QDebug>
#include <QTimer>

#include "server.h"

#define DEVICE_SERVER_PATH "/data/local/tmp/scrcpy-server.jar"
//#define SOCKET_NAME "qtscrcpy" //jar需要同步修改
#define SOCKET_NAME "scrcpy"

Server::Server(QObject *parent) : QObject(parent)
{
    connect(&m_workProcess, &AdbProcess::adbProcessResult, this, &Server::onWorkProcessResult);
    connect(&m_serverProcess, &AdbProcess::adbProcessResult, this, &Server::onWorkProcessResult);

    connect(&m_serverSocket, &QTcpServer::newConnection, this, [this](){
        m_deviceSocket = m_serverSocket.nextPendingConnection();
        connect(m_deviceSocket, &QTcpSocket::disconnected, m_deviceSocket, &QTcpSocket::deleteLater);
        //connect(m_deviceSocket, &QTcpSocket::error, m_deviceSocket, &QTcpSocket::deleteLater);
        connect(m_deviceSocket, &QTcpSocket::readyRead, this, [this](){
            static quint64 count = 0;
            qDebug() << count <<  "ready read";
            count++;
            QByteArray ar = m_deviceSocket->readAll();
            //m_deviceSocket->write(ar);
        });
    });

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
    AdbProcess* adb = new AdbProcess();
    if (!adb) {
        return false;
    }
    connect(adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        if (AdbProcess::AER_SUCCESS_START != processResult) {
            sender()->deleteLater();
        }
    });
    adb->removePath(m_serial, DEVICE_SERVER_PATH);
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
    AdbProcess* adb = new AdbProcess();
    if (!adb) {
        return false;
    }
    connect(adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        if (AdbProcess::AER_SUCCESS_START != processResult) {
            sender()->deleteLater();
        }
    });
    adb->reverseRemove(m_serial, SOCKET_NAME);
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
    AdbProcess* adb = new AdbProcess();
    if (!adb) {
        return false;
    }
    connect(adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        if (AdbProcess::AER_SUCCESS_START != processResult) {
            sender()->deleteLater();
        }
    });
    adb->forwardRemove(m_serial, m_localPort);
    return true;
}

bool Server::execute()
{
    if (m_serverProcess.isRuning()) {
        m_serverProcess.kill();
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
    if (!m_crop.isEmpty()) {
        args << m_crop;
    }

    // adb -s P7C0218510000537 shell CLASSPATH=/data/local/tmp/scrcpy-server.jar app_process / com.genymobile.scrcpy.Server 0 8000000 false
    // 这条adb命令是阻塞运行的，m_serverProcess进程不会退出了
    m_serverProcess.execute(m_serial, args);
    return true;
}

bool Server::start(const QString& serial, quint16 localPort, quint16 maxSize, quint32 bitRate, const QString& crop)
{
    if (serial.isEmpty()) {
        return false;
    }

    m_serial = serial;
    m_localPort = localPort;
    m_maxSize = maxSize;
    m_bitRate = bitRate;
    m_crop = crop;

    m_serverStartStep = SSS_PUSH;
    return startServerByStep();
}

void Server::connectTo()
{
    if (m_tunnelForward) {
        m_deviceSocket = new QTcpSocket(this);
        connect(m_deviceSocket, &QTcpSocket::disconnected, m_deviceSocket, &QTcpSocket::deleteLater);
        //connect(m_deviceSocket, &QTcpSocket::error, m_deviceSocket, &QTcpSocket::deleteLater);
        connect(m_deviceSocket, &QTcpSocket::readyRead, this, [this](){
            qDebug() << "ready read";
            m_deviceSocket->readAll();
        });
        m_deviceSocket->connectToHost(QHostAddress::LocalHost, m_localPort);
    }

    QTimer::singleShot(300, this, [this](){
        if (!m_deviceSocket) {
            emit connectToResult(false);
            return;
        }

        bool success = false;
        if (m_tunnelForward) {
            if (m_deviceSocket->isValid()) {
                // connect will success even if devices offline, recv data is real connect success
                // because connect is to pc adb server
                QByteArray ar = m_deviceSocket->read(1);
                if (!ar.isEmpty()) {
                    success = true;
                } else {
                    success = false;
                }
            } else {
                m_deviceSocket->deleteLater();
                success = false;
            }
        } else {
            if (m_deviceSocket->isValid()) {
                // we don't need the server socket anymore
                // just m_deviceSocket is ok
                m_serverSocket.close();
                success = true;
            } else {
                m_deviceSocket->deleteLater();
                success = false;
            }
        }
        if (success) {
            // the server is started, we can clean up the jar from the temporary folder
            removeServer();
            m_serverCopiedToDevice = false;
            // we don't need the adb tunnel anymore
            if (m_tunnelForward) {
                disableTunnelForward();
            } else {
                disableTunnelReverse();
            }
        }
        emit connectToResult(success);
    });
}

void Server::stop()
{
    // ignore failure
    m_serverProcess.kill();
    if (m_tunnelEnabled) {
        if (m_tunnelForward) {
            disableTunnelForward();
        } else {
            disableTunnelReverse();
        }
    }
    if (m_serverCopiedToDevice) {
        removeServer();
    }
    m_serverSocket.close();
    if (m_deviceSocket) {
        m_deviceSocket->disconnectFromHost();
    }
}

bool Server::startServerByStep()
{
    bool stepSuccess = false;
    // push, enable tunnel et start the server
    if (SSS_NULL != m_serverStartStep) {
        switch (m_serverStartStep) {
        case SSS_PUSH:
            stepSuccess = pushServer();
            break;
        case SSS_ENABLE_TUNNEL_REVERSE:
            stepSuccess = enableTunnelReverse();
            break;
        case SSS_ENABLE_TUNNEL_FORWARD:
            stepSuccess = enableTunnelForward();
            break;
        case SSS_EXECUTE_SERVER:
            // if "adb reverse" does not work (e.g. over "adb connect"), it fallbacks to
            // "adb forward", so the app socket is the client
            if (!m_tunnelForward) {
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
            stepSuccess = execute();
            break;
        default:
            break;
        }
    }

    if (!stepSuccess) {
        emit serverStartResult(false);
    }
    return stepSuccess;
}

void Server::onWorkProcessResult(AdbProcess::ADB_EXEC_RESULT processResult)
{
    if (sender() == &m_workProcess) {
        if (SSS_NULL != m_serverStartStep) {
            switch (m_serverStartStep) {
            case SSS_PUSH:
                if (AdbProcess::AER_SUCCESS_EXEC == processResult) {
                    m_serverCopiedToDevice = true;
                    m_serverStartStep = SSS_ENABLE_TUNNEL_REVERSE;
                    startServerByStep();
                } else if (AdbProcess::AER_SUCCESS_START != processResult){
                    qCritical("adb push");
                    m_serverStartStep = SSS_NULL;
                    emit serverStartResult(false);
                }
                break;
            case SSS_ENABLE_TUNNEL_REVERSE:
                if (AdbProcess::AER_SUCCESS_EXEC == processResult) {
                    m_serverStartStep = SSS_EXECUTE_SERVER;
                    startServerByStep();
                } else if (AdbProcess::AER_SUCCESS_START != processResult){
                    qCritical("adb reverse");
                    m_tunnelForward = true;
                    m_serverStartStep = SSS_ENABLE_TUNNEL_FORWARD;
                    startServerByStep();
                }
                break;
            case SSS_ENABLE_TUNNEL_FORWARD:
                if (AdbProcess::AER_SUCCESS_EXEC == processResult) {
                    m_serverStartStep = SSS_EXECUTE_SERVER;
                    startServerByStep();
                } else if (AdbProcess::AER_SUCCESS_START != processResult){
                    qCritical("adb forward");
                    m_serverStartStep = SSS_NULL;
                    emit serverStartResult(false);
                }
                break;
            default:
                break;
            }
        }
    }
    if (sender() == &m_serverProcess) {
        if (SSS_EXECUTE_SERVER == m_serverStartStep) {
            if (AdbProcess::AER_SUCCESS_START == processResult) {
                m_serverStartStep = SSS_NULL;
                m_tunnelEnabled = true;
                emit serverStartResult(true);
            } else if (AdbProcess::AER_ERROR_START == processResult){
                if (!m_tunnelForward) {
                    m_serverSocket.close();
                    disableTunnelReverse();
                } else {
                    disableTunnelForward();
                }
                qCritical("adb shell start server failed");
                m_serverStartStep = SSS_NULL;
                emit serverStartResult(false);
            }
        }
    }

}
