#ifndef CONVERT_H
#define CONVERT_H
#include <QtGlobal>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/frame.h"
}

class Convert
{
public:
    Convert();
    virtual ~Convert();

public:
    void setSrcFrameInfo(quint32 srcWidth, quint32 srcHeight, AVPixelFormat srcFormat);
    void getSrcFrameInfo(quint32& srcWidth, quint32& srcHeight, AVPixelFormat& srcFormat);
    void setDstFrameInfo(quint32 dstWidth, quint32 dstHeight, AVPixelFormat dstFormat);
    void getDstFrameInfo(quint32& dstWidth, quint32& dstHeight, AVPixelFormat& dstFormat);

    bool init();
    bool isInit();
    void deInit();
    bool convert(AVFrame* srcFrame, AVFrame* dstFrame);

private:
    quint32 m_srcWidth = 0;
    quint32 m_srcHeight = 0;
    AVPixelFormat m_srcFormat = AV_PIX_FMT_NONE;
    quint32 m_dstWidth = 0;
    quint32 m_dstHeight = 0;
    AVPixelFormat m_dstFormat = AV_PIX_FMT_NONE;

    struct SwsContext *m_convertCtx = Q_NULLPTR;
};

#endif // CONVERT_H
