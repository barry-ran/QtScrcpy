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
        // necessary
        QString serial = "";              // 设备序列号
        QString serverLocalPath = "";     // 本地安卓server路径

        // optional
        QString serverRemotePath = "/data/local/tmp/scrcpy-server.jar";    // 要推送到远端设备的server路径
        quint16 localPort = 27183;     // reverse时本地监听端口
        quint16 maxSize = 720;         // 视频分辨率
        quint32 bitRate = 8000000;     // 视频比特率
        quint32 maxFps = 60;           // 视频最大帧率
        bool useReverse = true;        // true:先使用adb reverse，失败后自动使用adb forward；false:直接使用adb forward
        int lockVideoOrientation = -1; // 是否锁定视频方向
        int stayAwake = false;         // 是否保持唤醒
        QString serverVersion = "1.21";// server版本
        QString logLevel = "info";     // log级别 debug/info/warn/error
        // 编码选项 ""表示默认
        // 例如 CodecOptions="profile=1,level=2"
        // 更多编码选项参考 https://d.android.com/reference/android/media/MediaFormat
        QString codecOptions = "";
        // 指定编码器名称(必须是H.264编码器)，""表示默认
        // 例如 CodecName="OMX.qcom.video.encoder.avc"
        QString codecName = "";

        QString crop = "";             // 视频裁剪
        bool control = true;           // 安卓端是否接收键鼠控制
    };

    explicit Server(QObject *parent = nullptr);
    virtual ~Server();

    bool start(Server::ServerParams params);
    void stop();
    bool isReverse();
    Server::ServerParams getParams();
    VideoSocket *getVideoSocket();
    QTcpSocket *getControlSocket();

signals:
    void serverStarted(bool success, const QString &deviceName = "", const QSize &size = QSize());
    void serverStoped();

private slots:
    void onWorkProcessResult(qsc::AdbProcess::ADB_EXEC_RESULT processResult);

protected:
    void timerEvent(QTimerEvent *event);

private:
    bool pushServer();
    bool enableTunnelReverse();
    bool disableTunnelReverse();
    bool enableTunnelForward();
    bool disableTunnelForward();
    bool execute();
    bool connectTo();
    bool startServerByStep();
    bool readInfo(VideoSocket *videoSocket, QString &deviceName, QSize &size);
    void startAcceptTimeoutTimer();
    void stopAcceptTimeoutTimer();
    void startConnectTimeoutTimer();
    void stopConnectTimeoutTimer();
    void onConnectTimer();

private:
    qsc::AdbProcess m_workProcess;
    qsc::AdbProcess m_serverProcess;
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
