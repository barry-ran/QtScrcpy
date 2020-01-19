#ifndef DEVICE_H
#define DEVICE_H

#include <QPointer>
#include <QTime>

class Recorder;
class Server;
class VideoBuffer;
class Decoder;
class FileHandler;
class Stream;
class VideoForm;
class Controller;
class Device : public QObject
{
    Q_OBJECT
public:
    struct DeviceParams {
        QString recordFileName = "";    // 视频录制文件名
        QString serial = "";            // 设备序列号
        quint16 localPort = 27183;      // reverse时本地监听端口
        quint16 maxSize = 720;          // 视频分辨率
        quint32 bitRate = 8000000;      // 视频比特率
        quint32 maxFps = 60;            // 视频最大帧率
        bool closeScreen = false;       // 启动时自动息屏
        bool useReverse = true;         // true:先使用adb reverse，失败后自动使用adb forward；false:直接使用adb forward
        bool display = true;            // 是否显示画面（或者仅仅后台录制）
        QString gameScript = "";        // 游戏映射脚本
    };
    explicit Device(DeviceParams params, QObject *parent = nullptr);
    virtual ~Device();

    VideoForm *getVideoForm();
    Controller *getController();
    Server *getServer();

    void updateScript(QString script);

signals:
    void deviceDisconnect(QString serial);

private:
    void initSignals();
    void startServer();

private:
    // server relevant
    QPointer<Server> m_server;
    QPointer<Decoder> m_decoder;
    QPointer<Controller> m_controller;
    QPointer<FileHandler> m_fileHandler;
    QPointer<Stream> m_stream;
    VideoBuffer* m_vb = Q_NULLPTR;
    Recorder* m_recorder = Q_NULLPTR;

    // ui
    QPointer<VideoForm> m_videoForm;

    QTime m_startTimeCount;
    DeviceParams m_params;
};

#endif // DEVICE_H
