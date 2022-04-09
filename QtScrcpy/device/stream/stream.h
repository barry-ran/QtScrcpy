#ifndef STREAM_H
#define STREAM_H

#include <QPointer>
#include <QThread>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class Stream : public QThread
{
    Q_OBJECT
public:
    Stream(std::function<qint32(quint8*, qint32)> recvData, QObject *parent = Q_NULLPTR);
    virtual ~Stream();

public:
    static bool init();
    static void deInit();

    bool startDecode();
    void stopDecode();

signals:
    void onStreamStop();
    void getFrame(AVPacket* packet);
    void getConfigFrame(AVPacket* packet);

protected:
    void run();
    bool recvPacket(AVPacket *packet);
    bool pushPacket(AVPacket *packet);
    bool processConfigPacket(AVPacket *packet);
    bool parse(AVPacket *packet);
    bool processFrame(AVPacket *packet);
    qint32 recvData(quint8 *buf, qint32 bufSize);

private:
    std::function<qint32(quint8*, qint32)> m_recvData = nullptr;

    AVCodecContext *m_codecCtx = Q_NULLPTR;
    AVCodecParserContext *m_parser = Q_NULLPTR;
    // successive packets may need to be concatenated, until a non-config
    // packet is available
    bool m_hasPending = false;
    AVPacket m_pending;
};

#endif // STREAM_H
