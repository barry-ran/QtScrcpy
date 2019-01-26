#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QPointer>

#include "tcpserver.h"
#include "devicesocket.h"
#include "adbprocess.h"

class Server : public QObject
{
    Q_OBJECT

    enum SERVER_START_STEP {
        SSS_NULL,
        SSS_PUSH,
        SSS_ENABLE_TUNNEL_REVERSE,
        SSS_ENABLE_TUNNEL_FORWARD,
        SSS_EXECUTE_SERVER,
        SSS_RUNNING,
    };
public:
    explicit Server(QObject *parent = nullptr);
    virtual ~Server();

    bool start(const QString& serial, quint16 localPort, quint16 maxSize, quint32 bitRate, const QString& crop, bool sendFrameMeta = false);
    bool connectTo();

    DeviceSocket* getDeviceSocket();

    void stop();

signals:
    void serverStartResult(bool success);
    void connectToResult(bool success, const QString &deviceName, const QSize &size);
    void onServerStop();

private slots:
    void onWorkProcessResult(AdbProcess::ADB_EXEC_RESULT processResult);

protected:
    void timerEvent(QTimerEvent *event);

private:
    const QString& getServerPath();
    bool pushServer();
    bool removeServer();
    bool enableTunnelReverse();
    bool disableTunnelReverse();
    bool enableTunnelForward();
    bool disableTunnelForward();
    bool execute();
    bool startServerByStep();
    bool readInfo(QString& deviceName, QSize& size);
    void startAcceptTimeoutTimer();
    void stopAcceptTimeoutTimer();

private:
    QString m_serverPath = "";
    AdbProcess m_workProcess;
    QString m_serial = "";
    AdbProcess m_serverProcess;
    TcpServer m_serverSocket; // only used if !tunnel_forward
    QPointer<DeviceSocket> m_deviceSocket = Q_NULLPTR;
    quint16 m_localPort = 0;
    bool m_tunnelEnabled = false;
    bool m_tunnelForward = false; // use "adb forward" instead of "adb reverse"
    bool m_sendFrameMeta = false;
    quint16 m_maxSize = 0;
    quint32 m_bitRate = 0;
    QString m_crop = "";
    quint32 m_acceptTimeoutTimer = 0;

    SERVER_START_STEP m_serverStartStep = SSS_NULL;
};

#endif // SERVER_H
