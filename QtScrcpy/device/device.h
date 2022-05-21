#ifndef DEVICE_H
#define DEVICE_H

#include <QElapsedTimer>
#include <QPointer>
#include <QTime>

#include "controlmsg.h"

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class Recorder;
class Server;
class VideoBuffer;
class Decoder;
class FileHandler;
class Stream;
class VideoForm;
class Controller;
struct AVFrame;
class Device : public QObject
{
    Q_OBJECT
public:
    struct DeviceParams
    {
        QString serverLocalPath = "";     // 本地安卓server路径
        QString serverRemotePath = "";    // 要推送到远端设备的server路径
        QString recordFileName = "";      // 视频录制文件名
        QString recordPath = "";          // 视频保存路径
        QString serial = "";              // 设备序列号
        quint16 localPort = 27183;        // reverse时本地监听端口
        quint16 maxSize = 720;            // 视频分辨率
        quint32 bitRate = 2000000;        // 视频比特率
        quint32 maxFps = 60;              // 视频最大帧率
        bool closeScreen = false;         // 启动时自动息屏
        bool useReverse = true;           // true:先使用adb reverse，失败后自动使用adb forward；false:直接使用adb forward
        bool display = true;              // 是否显示画面（或者仅仅后台录制）
        QString gameScript = "";          // 游戏映射脚本
        bool renderExpiredFrames = false; // 是否渲染延迟视频帧
        int lockVideoOrientation = -1;    // 是否锁定视频方向
        bool stayAwake = false;           // 是否保持唤醒
        bool framelessWindow = false;     // 是否无边框窗口
    };
    explicit Device(DeviceParams params, QObject *parent = nullptr);
    virtual ~Device();

    bool connectDevice();
    void disconnectDevice();
    void postGoBack();
    void postGoHome();
    void postGoMenu();
    void postAppSwitch();
    void postPower();
    void postVolumeUp();
    void postVolumeDown();
    void postCopy();
    void postCut();
    void setScreenPowerMode(ControlMsg::ScreenPowerMode mode);
    void expandNotificationPanel();
    void collapsePanel();
    void postBackOrScreenOn(bool down);
    void postTextInput(QString &text);
    void requestDeviceClipboard();
    void setDeviceClipboard(bool pause = true);
    void clipboardPaste();
    void pushFileRequest(const QString &file, const QString &devicePath = "");
    void installApkRequest(const QString &apkFile);

    // key map
    void mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize);
    void wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize);
    void keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize);
    

    void screenshot();
    void showTouch(bool show);
    void grabCursor(bool grab);

    VideoForm *getVideoForm();
    Server *getServer();
    const QString &getSerial();
    const QSize frameSize();

    void updateScript(QString script);
    bool isCurrentCustomKeymap();

signals:
    void deviceConnected(bool success, const QString& serial, const QString& deviceName, const QSize& size);
    void deviceDisconnected(QString serial);

private:
    void initSignals();
    bool saveFrame(int width, int height, uint8_t* dataRGB32);

private:
    // server relevant
    QPointer<Server> m_server;
    QPointer<Decoder> m_decoder;
    QPointer<Controller> m_controller;
    QPointer<FileHandler> m_fileHandler;
    QPointer<Stream> m_stream;
    QPointer<Recorder> m_recorder = Q_NULLPTR;

    // ui
    QPointer<VideoForm> m_videoForm;

    QElapsedTimer m_startTimeCount;
    DeviceParams m_params;
};

#endif // DEVICE_H
