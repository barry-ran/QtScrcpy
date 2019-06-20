#ifndef DEVICE_H
#define DEVICE_H

#include <QPointer>
#include <QTime>

#include "controller.h"

class Recorder;
class Server;
class VideoBuffer;
class Decoder;
class FileHandler;
class Stream;
class VideoForm;
class Device : public QObject
{
    Q_OBJECT
public:
    struct DeviceParams {
        QString recordFileName = "";
        QString serial = "";
        quint16 localPort = 27183;
        quint16 maxSize = 0;
        quint32 bitRate = 8000000;
        bool closeScreen = false;
    };
    explicit Device(DeviceParams params, QObject *parent = nullptr);
    virtual ~Device();

    VideoForm *getVideoForm();

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
