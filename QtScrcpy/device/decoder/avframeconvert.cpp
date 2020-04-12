#include <QDebug>

#include "avframeconvert.h"

AVFrameConvert::AVFrameConvert() {}

AVFrameConvert::~AVFrameConvert() {}

void AVFrameConvert::setSrcFrameInfo(int srcWidth, int srcHeight, AVPixelFormat srcFormat)
{
    m_srcWidth = srcWidth;
    m_srcHeight = srcHeight;
    m_srcFormat = srcFormat;
    qDebug() << "Convert::src frame info " << srcWidth << "x" << srcHeight;
}

void AVFrameConvert::getSrcFrameInfo(int &srcWidth, int &srcHeight, AVPixelFormat &srcFormat)
{
    srcWidth = m_srcWidth;
    srcHeight = m_srcHeight;
    srcFormat = m_srcFormat;
}

void AVFrameConvert::setDstFrameInfo(int dstWidth, int dstHeight, AVPixelFormat dstFormat)
{
    m_dstWidth = dstWidth;
    m_dstHeight = dstHeight;
    m_dstFormat = dstFormat;
}

void AVFrameConvert::getDstFrameInfo(int &dstWidth, int &dstHeight, AVPixelFormat &dstFormat)
{
    dstWidth = m_dstWidth;
    dstHeight = m_dstHeight;
    dstFormat = m_dstFormat;
}

bool AVFrameConvert::init()
{
    if (m_convertCtx) {
        return true;
    }
    m_convertCtx = sws_getContext(m_srcWidth, m_srcHeight, m_srcFormat, m_dstWidth, m_dstHeight, m_dstFormat, SWS_BICUBIC, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);
    if (!m_convertCtx) {
        return false;
    }
    return true;
}

bool AVFrameConvert::isInit()
{
    return m_convertCtx ? true : false;
}

void AVFrameConvert::deInit()
{
    if (m_convertCtx) {
        sws_freeContext(m_convertCtx);
        m_convertCtx = Q_NULLPTR;
    }
}

bool AVFrameConvert::convert(const AVFrame *srcFrame, AVFrame *dstFrame)
{
    if (!m_convertCtx || !srcFrame || !dstFrame) {
        return false;
    }
    qint32 ret
        = sws_scale(m_convertCtx, static_cast<const uint8_t *const *>(srcFrame->data), srcFrame->linesize, 0, m_srcHeight, dstFrame->data, dstFrame->linesize);
    if (0 == ret) {
        return false;
    }
    return true;
}
