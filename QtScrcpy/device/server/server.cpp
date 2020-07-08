#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QThread>
#include <QTimer>
#include <QTimerEvent>

#include "config.h"
#include "server.h"

#define DEVICE_NAME_FIELD_LENGTH 64
#define SOCKET_NAME "scrcpy"
#define MAX_CONNECT_COUNT 30
#define MAX_RESTART_COUNT 1

Server::Server(QObject *parent) : QObject(parent)
{
    connect(&m_workProcess, &AdbProcess::adbProcessResult, this, &Server::onWorkProcessResult);
    connect(&m_serverProcess, &AdbProcess::adbProcessResult, this, &Server::onWorkProcessResult);

    connect(&m_serverSocket, &QTcpServer::newConnection, this, [this]() {
        QTcpSocket *tmp = m_serverSocket.nextPendingConnection();
        if (dynamic_cast<VideoSocket *>(tmp)) {
            m_videoSocket = dynamic_cast<VideoSocket *>(tmp);
            if (!m_videoSocket->isValid() || !readInfo(m_videoSocket, m_deviceName, m_deviceSize)) {
                stop();
                emit connectToResult(false);
            }
        } else {
            m_controlSocket = tmp;
            if (m_controlSocket && m_controlSocket->isValid()) {
                // we don't need the server socket anymore
                // just m_videoSocket is ok
                m_serverSocket.close();
                // we don't need the adb tunnel anymore
                disableTunnelReverse();
                m_tunnelEnabled = false;
                emit connectToResult(true, m_deviceName, m_deviceSize);
            } else {
                stop();
                emit connectToResult(false);
            }
            stopAcceptTimeoutTimer();
        }
    });
}

Server::~Server() {}

const QString &Server::getServerPath()
{
    if (m_serverPath.isEmpty()) {
        m_serverPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_SERVER_PATH"));
        QFileInfo fileInfo(m_serverPath);
        if (m_serverPath.isEmpty() || !fileInfo.isFile()) {
            m_serverPath = QCoreApplication::applicationDirPath() + "/scrcpy-server";
        }
    }
    return m_serverPath;
}

bool Server::pushServer()
{
    if (m_workProcess.isRuning()) {
        m_workProcess.kill();
    }
    m_workProcess.push(m_params.serial, getServerPath(), Config::getInstance().getServerPath());
    return true;
}

bool Server::enableTunnelReverse()
{
    if (m_workProcess.isRuning()) {
        m_workProcess.kill();
    }
    m_workProcess.reverse(m_params.serial, SOCKET_NAME, m_params.localPort);
    return true;
}

bool Server::disableTunnelReverse()
{
    AdbProcess *adb = new AdbProcess();
    if (!adb) {
        return false;
    }
    connect(adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult) {
        if (AdbProcess::AER_SUCCESS_START != processResult) {
            sender()->deleteLater();
        }
    });
    adb->reverseRemove(m_params.serial, SOCKET_NAME);
    return true;
}

bool Server::enableTunnelForward()
{
    if (m_workProcess.isRuning()) {
        m_workProcess.kill();
    }
    m_workProcess.forward(m_params.serial, m_params.localPort, SOCKET_NAME);
    return true;
}
bool Server::disableTunnelForward()
{
    AdbProcess *adb = new AdbProcess();
    if (!adb) {
        return false;
    }
    connect(adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult) {
        if (AdbProcess::AER_SUCCESS_START != processResult) {
            sender()->deleteLater();
        }
    });
    adb->forwardRemove(m_params.serial, m_params.localPort);
    return true;
}

bool Server::execute()
{
    if (m_serverProcess.isRuning()) {
        m_serverProcess.kill();
    }
    QStringList args;
    args << "shell";
    args << QString("CLASSPATH=%1").arg(Config::getInstance().getServerPath());
    args << "app_process";

#ifdef SERVER_DEBUGGER
#define SERVER_DEBUGGER_PORT "5005"

    args <<
#ifdef SERVER_DEBUGGER_METHOD_NEW
        /* Android 9 and above */
        "-XjdwpProvider:internal -XjdwpOptions:transport=dt_socket,suspend=y,server=y,address="
#else
        /* Android 8 and below */
        "-agentlib:jdwp=transport=dt_socket,suspend=y,server=y,address="
#endif
        SERVER_DEBUGGER_PORT,
#endif

        args << "/"; // unused;
    args << "com.genymobile.scrcpy.Server";
    args << Config::getInstance().getServerVersion();
    args << Config::getInstance().getLogLevel();
    args << QString::number(m_params.maxSize);
    args << QString::number(m_params.bitRate);
    args << QString::number(m_params.maxFps);
    args << QString::number(m_params.lockVideoOrientation);
    args << (m_tunnelForward ? "true" : "false");
    if (m_params.crop.isEmpty()) {
        args << "-";
    } else {
        args << m_params.crop;
    }
    args << "true"; // always send frame meta (packet boundaries + timestamp)
    args << (m_params.control ? "true" : "false");
    args << "0";                                     // display id
    args << "false";                                 // show touch
    args << (m_params.stayAwake ? "true" : "false"); // stay awake
    // code option
    // https://github.com/Genymobile/scrcpy/commit/080a4ee3654a9b7e96c8ffe37474b5c21c02852a
    // <https://d.android.com/reference/android/media/MediaFormat>
    args << Config::getInstance().getCodecOptions();

#ifdef SERVER_DEBUGGER
    qInfo("Server debugger waiting for a client on device port " SERVER_DEBUGGER_PORT "...");
    // From the computer, run
    //     adb forward tcp:5005 tcp:5005
    // Then, from Android Studio: Run > Debug > Edit configurations...
    // On the left, click on '+', "Remote", with:
    //     Host: localhost
    //     Port: 5005
    // Then click on "Debug"
#endif

    // adb -s P7C0218510000537 shell CLASSPATH=/data/local/tmp/scrcpy-server app_process / com.genymobile.scrcpy.Server 0 8000000 false
    // mark: crop input format: "width:height:x:y" or - for no crop, for example: "100:200:0:0"
    // 这条adb命令是阻塞运行的，m_serverProcess进程不会退出了
    m_serverProcess.execute(m_params.serial, args);
    return true;
}

bool Server::start(Server::ServerParams params)
{
    m_params = params;
    m_serverStartStep = SSS_PUSH;
    return startServerByStep();
}

bool Server::connectTo()
{
    if (SSS_RUNNING != m_serverStartStep) {
        qWarning("server not run");
        return false;
    }

    if (!m_tunnelForward && !m_videoSocket) {
        startAcceptTimeoutTimer();
        return true;
    }

    startConnectTimeoutTimer();
    return true;
}

bool Server::isReverse()
{
    return !m_tunnelForward;
}

Server::ServerParams Server::getParams()
{
    return m_params;
}

void Server::timerEvent(QTimerEvent *event)
{
    if (event && m_acceptTimeoutTimer == event->timerId()) {
        stopAcceptTimeoutTimer();
        emit connectToResult(false, "", QSize());
    } else if (event && m_connectTimeoutTimer == event->timerId()) {
        onConnectTimer();
    }
}

VideoSocket *Server::getVideoSocket()
{
    return m_videoSocket;
}

QTcpSocket *Server::getControlSocket()
{
    return m_controlSocket;
}

void Server::stop()
{
    if (m_tunnelForward) {
        stopConnectTimeoutTimer();
    } else {
        stopAcceptTimeoutTimer();
    }

    if (m_videoSocket) {
        m_videoSocket->close();
        m_videoSocket->deleteLater();
    }
    if (m_controlSocket) {
        m_controlSocket->close();
        m_controlSocket->deleteLater();
    }
    // ignore failure
    m_serverProcess.kill();
    if (m_tunnelEnabled) {
        if (m_tunnelForward) {
            disableTunnelForward();
        } else {
            disableTunnelReverse();
        }
        m_tunnelForward = false;
        m_tunnelEnabled = false;
    }
    m_serverSocket.close();
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

bool Server::readInfo(VideoSocket *videoSocket, QString &deviceName, QSize &size)
{
    unsigned char buf[DEVICE_NAME_FIELD_LENGTH + 4];
    if (videoSocket->bytesAvailable() <= (DEVICE_NAME_FIELD_LENGTH + 4)) {
        videoSocket->waitForReadyRead(300);
    }

    qint64 len = videoSocket->read((char *)buf, sizeof(buf));
    if (len < DEVICE_NAME_FIELD_LENGTH + 4) {
        qInfo("Could not retrieve device information");
        return false;
    }
    buf[DEVICE_NAME_FIELD_LENGTH - 1] = '\0'; // in case the client sends garbage
    // strcpy is safe here, since name contains at least DEVICE_NAME_FIELD_LENGTH bytes
    // and strlen(buf) < DEVICE_NAME_FIELD_LENGTH
    deviceName = (char *)buf;
    size.setWidth((buf[DEVICE_NAME_FIELD_LENGTH] << 8) | buf[DEVICE_NAME_FIELD_LENGTH + 1]);
    size.setHeight((buf[DEVICE_NAME_FIELD_LENGTH + 2] << 8) | buf[DEVICE_NAME_FIELD_LENGTH + 3]);
    return true;
}

void Server::startAcceptTimeoutTimer()
{
    stopAcceptTimeoutTimer();
    m_acceptTimeoutTimer = startTimer(1000);
}

void Server::stopAcceptTimeoutTimer()
{
    if (m_acceptTimeoutTimer) {
        killTimer(m_acceptTimeoutTimer);
        m_acceptTimeoutTimer = 0;
    }
}

void Server::startConnectTimeoutTimer()
{
    stopConnectTimeoutTimer();
    m_connectTimeoutTimer = startTimer(100);
}

void Server::stopConnectTimeoutTimer()
{
    if (m_connectTimeoutTimer) {
        killTimer(m_connectTimeoutTimer);
        m_connectTimeoutTimer = 0;
    }
    m_connectCount = 0;
}

void Server::onConnectTimer()
{
    // device server need time to start
    // 这里连接太早时间不够导致安卓监听socket还没有建立，readInfo会失败，所以采取定时重试策略
    // 每隔100ms尝试一次，最多尝试MAX_CONNECT_COUNT次
    QString deviceName;
    QSize deviceSize;
    bool success = false;

    VideoSocket *videoSocket = new VideoSocket();
    QTcpSocket *controlSocket = new QTcpSocket();

    videoSocket->connectToHost(QHostAddress::LocalHost, m_params.localPort);
    if (!videoSocket->waitForConnected(1000)) {
        // 连接到adb很快的，这里失败不重试
        m_connectCount = MAX_CONNECT_COUNT;
        qWarning("video socket connect to server failed");
        goto result;
    }

    controlSocket->connectToHost(QHostAddress::LocalHost, m_params.localPort);
    if (!controlSocket->waitForConnected(1000)) {
        // 连接到adb很快的，这里失败不重试
        m_connectCount = MAX_CONNECT_COUNT;
        qWarning("control socket connect to server failed");
        goto result;
    }

    if (QTcpSocket::ConnectedState == videoSocket->state()) {
        // connect will success even if devices offline, recv data is real connect success
        // because connect is to pc adb server
        videoSocket->waitForReadyRead(1000);
        // devices will send 1 byte first on tunnel forward mode
        QByteArray data = videoSocket->read(1);
        if (!data.isEmpty() && readInfo(videoSocket, deviceName, deviceSize)) {
            success = true;
            goto result;
        } else {
            qWarning("video socket connect to server read device info failed, try again");
            goto result;
        }
    } else {
        qWarning("connect to server failed");
        m_connectCount = MAX_CONNECT_COUNT;
        goto result;
    }

result:
    if (success) {
        stopConnectTimeoutTimer();
        m_videoSocket = videoSocket;
        m_controlSocket = controlSocket;
        // we don't need the adb tunnel anymore
        disableTunnelForward();
        m_tunnelEnabled = false;
        m_restartCount = 0;
        emit connectToResult(success, deviceName, deviceSize);
        return;
    }

    if (videoSocket) {
        videoSocket->deleteLater();
    }
    if (controlSocket) {
        controlSocket->deleteLater();
    }

    if (MAX_CONNECT_COUNT <= m_connectCount++) {
        stopConnectTimeoutTimer();
        stop();
        if (MAX_RESTART_COUNT > m_restartCount++) {
            qWarning("restart server auto");
            start(m_params);
        } else {
            m_restartCount = 0;
            emit connectToResult(false);
        }
    }
}

void Server::onWorkProcessResult(AdbProcess::ADB_EXEC_RESULT processResult)
{
    if (sender() == &m_workProcess) {
        if (SSS_NULL != m_serverStartStep) {
            switch (m_serverStartStep) {
            case SSS_PUSH:
                if (AdbProcess::AER_SUCCESS_EXEC == processResult) {
                    if (m_params.useReverse) {
                        m_serverStartStep = SSS_ENABLE_TUNNEL_REVERSE;
                    } else {
                        m_tunnelForward = true;
                        m_serverStartStep = SSS_ENABLE_TUNNEL_FORWARD;
                    }
                    startServerByStep();
                } else if (AdbProcess::AER_SUCCESS_START != processResult) {
                    qCritical("adb push failed");
                    m_serverStartStep = SSS_NULL;
                    emit serverStartResult(false);
                }
                break;
            case SSS_ENABLE_TUNNEL_REVERSE:
                if (AdbProcess::AER_SUCCESS_EXEC == processResult) {
                    // At the application level, the device part is "the server" because it
                    // serves video stream and control. However, at the network level, the
                    // client listens and the server connects to the client. That way, the
                    // client can listen before starting the server app, so there is no need to
                    // try to connect until the server socket is listening on the device.
                    m_serverSocket.setMaxPendingConnections(2);
                    if (!m_serverSocket.listen(QHostAddress::LocalHost, m_params.localPort)) {
                        qCritical() << QString("Could not listen on port %1").arg(m_params.localPort).toStdString().c_str();
                        m_serverStartStep = SSS_NULL;
                        disableTunnelReverse();
                        emit serverStartResult(false);
                        break;
                    }

                    m_serverStartStep = SSS_EXECUTE_SERVER;
                    startServerByStep();
                } else if (AdbProcess::AER_SUCCESS_START != processResult) {
                    // 有一些设备reverse会报错more than o'ne device，adb的bug
                    // https://github.com/Genymobile/scrcpy/issues/5
                    qCritical("adb reverse failed");
                    m_tunnelForward = true;
                    m_serverStartStep = SSS_ENABLE_TUNNEL_FORWARD;
                    startServerByStep();
                }
                break;
            case SSS_ENABLE_TUNNEL_FORWARD:
                if (AdbProcess::AER_SUCCESS_EXEC == processResult) {
                    m_serverStartStep = SSS_EXECUTE_SERVER;
                    startServerByStep();
                } else if (AdbProcess::AER_SUCCESS_START != processResult) {
                    qCritical("adb forward failed");
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
                m_serverStartStep = SSS_RUNNING;
                m_tunnelEnabled = true;
                emit serverStartResult(true);
            } else if (AdbProcess::AER_ERROR_START == processResult) {
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
        } else if (SSS_RUNNING == m_serverStartStep) {
            m_serverStartStep = SSS_NULL;
            emit onServerStop();
        }
    }
}
