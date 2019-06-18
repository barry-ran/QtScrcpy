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
    Decoder();
    virtual ~Decoder();

    void setVideoBuffer(VideoBuffer* vb);
    bool open(const AVCodec *codec);
    void close();
    bool push(const AVPacket *packet);
    void interrupt();

signals:
    void onNewFrame();

protected:
    void pushFrame();

private:    
    VideoBuffer* m_vb = Q_NULLPTR;
    AVCodecContext* m_codecCtx = Q_NULLPTR;
    bool m_isCodecCtxOpen = false;
};

#endif // DECODER_H
