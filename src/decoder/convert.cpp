#include <QDebug>

#include "convert.h"

Convert::Convert()
{

}

Convert::~Convert()
{

}

void Convert::setSrcFrameInfo(quint32 srcWidth, quint32 srcHeight, AVPixelFormat srcFormat)
{
    m_srcWidth = srcWidth;
    m_srcHeight = srcHeight;
    m_srcFormat = srcFormat;
    qDebug() << "Convert::src frame info " << srcWidth << "x" << srcHeight;
}

void Convert::getSrcFrameInfo(quint32& srcWidth, quint32& srcHeight, AVPixelFormat& srcFormat)
{
    srcWidth = m_srcWidth;
    srcHeight = m_srcHeight;
    srcFormat = m_srcFormat;    
}

void Convert::setDstFrameInfo(quint32 dstWidth, quint32 dstHeight, AVPixelFormat dstFormat)
{
    m_dstWidth = dstWidth;
    m_dstHeight = dstHeight;
    m_dstFormat = dstFormat;
}

void Convert::getDstFrameInfo(quint32& dstWidth, quint32& dstHeight, AVPixelFormat& dstFormat)
{
    dstWidth = m_dstWidth;
    dstHeight = m_dstHeight;
    dstFormat = m_dstFormat;
}

bool Convert::init()
{
    if (m_convertCtx) {
        return true;
    }
    m_convertCtx = sws_getContext(m_srcWidth, m_srcHeight, m_srcFormat, m_dstWidth, m_dstHeight, m_dstFormat,
                                  SWS_BICUBIC, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);
    if (!m_convertCtx) {
        return false;
    }
    return true;
}

bool Convert::isInit()
{
    return m_convertCtx ? true : false;
}

void Convert::deInit()
{
    if (m_convertCtx) {
        sws_freeContext(m_convertCtx);
        m_convertCtx = Q_NULLPTR;
    }
}

bool Convert::convert(AVFrame* srcFrame, AVFrame* dstFrame)
{
    if(!m_convertCtx || !srcFrame || !dstFrame) {
        return false;
    }
    qint32 ret = sws_scale(m_convertCtx, (const uint8_t* const*)srcFrame->data, srcFrame->linesize, 0, m_srcHeight, dstFrame->data, dstFrame->linesize);    
    if (0 == ret) {
        return false;
    }
    return true;
}
