#ifndef AVFRAMECONVERT_H
#define AVFRAMECONVERT_H
#include <QtGlobal>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
}

class AVFrameConvert
{
public:
    AVFrameConvert();
    virtual ~AVFrameConvert();

public:
    void setSrcFrameInfo(int srcWidth, int srcHeight, AVPixelFormat srcFormat);
    void getSrcFrameInfo(int &srcWidth, int &srcHeight, AVPixelFormat &srcFormat);
    void setDstFrameInfo(int dstWidth, int dstHeight, AVPixelFormat dstFormat);
    void getDstFrameInfo(int &dstWidth, int &dstHeight, AVPixelFormat &dstFormat);

    bool init();
    bool isInit();
    void deInit();
    bool convert(const AVFrame *srcFrame, AVFrame *dstFrame);

private:
    int m_srcWidth = 0;
    int m_srcHeight = 0;
    AVPixelFormat m_srcFormat = AV_PIX_FMT_NONE;
    int m_dstWidth = 0;
    int m_dstHeight = 0;
    AVPixelFormat m_dstFormat = AV_PIX_FMT_NONE;

    struct SwsContext *m_convertCtx = Q_NULLPTR;
};

#endif // AVFRAMECONVERT_H
