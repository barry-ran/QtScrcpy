#ifndef DECODER_H
#define DECODER_H
#include <QObject>

extern "C"
{
#include "libavcodec/avcodec.h"
}

class VideoBuffer;
class Decoder : public QObject
{
    Q_OBJECT
public:
    Decoder(VideoBuffer *vb, QObject *parent = Q_NULLPTR);
    virtual ~Decoder();

    bool open(const AVCodec *codec);
    void close();
    bool push(const AVPacket *packet);
    void interrupt();

signals:
    void onNewFrame();

protected:
    void pushFrame();

private:
    VideoBuffer *m_vb = Q_NULLPTR;
    AVCodecContext *m_codecCtx = Q_NULLPTR;
    bool m_isCodecCtxOpen = false;
};

#endif // DECODER_H
