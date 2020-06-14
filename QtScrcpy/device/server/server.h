#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QPointer>
#include <QSize>

#include "adbprocess.h"
#include "tcpserver.h"
#include "videosocket.h"

class Server : public QObject
{
    Q_OBJECT

    enum SERVER_START_STEP
    {
        SSS_NULL,
        SSS_PUSH,
        SSS_ENABLE_TUNNEL_REVERSE,
        SSS_ENABLE_TUNNEL_FORWARD,
        SSS_EXECUTE_SERVER,
        SSS_RUNNING,
    };

public:
    struct ServerParams
    {
        QString serial = "";           // 设备序列号
        quint16 localPort = 27183;     // reverse时本地监听端口
        quint16 maxSize = 720;         // 视频分辨率
        quint32 bitRate = 8000000;     // 视频比特率
        quint32 maxFps = 60;           // 视频最大帧率
        QString crop = "-";            // 视频裁剪
        bool control = true;           // 安卓端是否接收键鼠控制
        bool useReverse = true;        // true:先使用adb reverse，失败后自动使用adb forward；false:直接使用adb forward
        int lockVideoOrientation = -1; // 是否锁定视频方向
        int stayAwake = false;         // 是否保持唤醒
    };

    explicit Server(QObject *parent = nullptr);
    virtual ~Server();

    bool start(Server::ServerParams params);
    bool connectTo();
    bool isReverse();
    Server::ServerParams getParams();

    VideoSocket *getVideoSocket();
    QTcpSocket *getControlSocket();

    void stop();

signals:
    void serverStartResult(bool success);
    void connectToResult(bool success, const QString &deviceName = "", const QSize &size = QSize());
    void onServerStop();

private slots:
    void onWorkProcessResult(AdbProcess::ADB_EXEC_RESULT processResult);

protected:
    void timerEvent(QTimerEvent *event);

private:
    const QString &getServerPath();
    bool pushServer();
    bool enableTunnelReverse();
    bool disableTunnelReverse();
    bool enableTunnelForward();
    bool disableTunnelForward();
    bool execute();
    bool startServerByStep();
    bool readInfo(VideoSocket *videoSocket, QString &deviceName, QSize &size);
    void startAcceptTimeoutTimer();
    void stopAcceptTimeoutTimer();
    void startConnectTimeoutTimer();
    void stopConnectTimeoutTimer();
    void onConnectTimer();

private:
    QString m_serverPath = "";
    AdbProcess m_workProcess;
    AdbProcess m_serverProcess;
    TcpServer m_serverSocket; // only used if !tunnel_forward
    QPointer<VideoSocket> m_videoSocket = Q_NULLPTR;
    QPointer<QTcpSocket> m_controlSocket = Q_NULLPTR;
    bool m_tunnelEnabled = false;
    bool m_tunnelForward = false; // use "adb forward" instead of "adb reverse"
    int m_acceptTimeoutTimer = 0;
    int m_connectTimeoutTimer = 0;
    quint32 m_connectCount = 0;
    quint32 m_restartCount = 0;
    QString m_deviceName = "";
    QSize m_deviceSize = QSize();
    ServerParams m_params;

    SERVER_START_STEP m_serverStartStep = SSS_NULL;
};

#endif // SERVER_H
