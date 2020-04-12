#ifndef STREAM_H
#define STREAM_H

#include <QPointer>
#include <QThread>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class VideoSocket;
class Recorder;
class Decoder;
class Stream : public QThread
{
    Q_OBJECT
public:
    Stream(QObject *parent = Q_NULLPTR);
    virtual ~Stream();

public:
    static bool init();
    static void deInit();

    void setDecoder(Decoder *decoder);
    void setRecoder(Recorder *recorder);
    void setVideoSocket(VideoSocket *deviceSocket);
    qint32 recvData(quint8 *buf, qint32 bufSize);
    bool startDecode();
    void stopDecode();

signals:
    void onStreamStop();

protected:
    void run();
    bool recvPacket(AVPacket *packet);
    bool pushPacket(AVPacket *packet);
    bool processConfigPacket(AVPacket *packet);
    bool parse(AVPacket *packet);
    bool processFrame(AVPacket *packet);

private:
    QPointer<VideoSocket> m_videoSocket;
    // for recorder
    Recorder *m_recorder = Q_NULLPTR;
    Decoder *m_decoder = Q_NULLPTR;

    AVCodecContext *m_codecCtx = Q_NULLPTR;
    AVCodecParserContext *m_parser = Q_NULLPTR;
    // successive packets may need to be concatenated, until a non-config
    // packet is available
    bool m_hasPending = false;
    AVPacket m_pending;
};

#endif // STREAM_H
