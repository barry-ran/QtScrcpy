#include "videobuffer.h"
#include "avframeconvert.h"
extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
}

VideoBuffer::VideoBuffer(QObject *parent) : QObject(parent) {
    connect(&m_fpsCounter, &FpsCounter::updateFPS, this, &VideoBuffer::updateFPS);
}

VideoBuffer::~VideoBuffer() {}

bool VideoBuffer::init()
{
    m_decodingFrame = av_frame_alloc();
    if (!m_decodingFrame) {
        goto error;
    }

    m_renderingframe = av_frame_alloc();
    if (!m_renderingframe) {
        goto error;
    }

    // there is initially no rendering frame, so consider it has already been
    // consumed
    m_renderingFrameConsumed = true;

    m_fpsCounter.start();
    return true;

error:
    deInit();
    return false;
}

void VideoBuffer::deInit()
{
    if (m_decodingFrame) {
        av_frame_free(&m_decodingFrame);
        m_decodingFrame = Q_NULLPTR;
    }
    if (m_renderingframe) {
        av_frame_free(&m_renderingframe);
        m_renderingframe = Q_NULLPTR;
    }
    m_fpsCounter.stop();
}

void VideoBuffer::lock()
{
    m_mutex.lock();
}

void VideoBuffer::unLock()
{
    m_mutex.unlock();
}

void VideoBuffer::setRenderExpiredFrames(bool renderExpiredFrames)
{
    m_renderExpiredFrames = renderExpiredFrames;
}

AVFrame *VideoBuffer::decodingFrame()
{
    return m_decodingFrame;
}

void VideoBuffer::offerDecodedFrame(bool &previousFrameSkipped)
{
    m_mutex.lock();

    if (m_renderExpiredFrames) {
        // if m_renderExpiredFrames is enable, then the decoder must wait for the current
        // frame to be consumed
        while (!m_renderingFrameConsumed && !m_interrupted) {
            m_renderingFrameConsumedCond.wait(&m_mutex);
        }
    } else {
        if (m_fpsCounter.isStarted() && !m_renderingFrameConsumed) {
            m_fpsCounter.addSkippedFrame();
        }
    }

    swap();
    previousFrameSkipped = !m_renderingFrameConsumed;
    m_renderingFrameConsumed = false;
    m_mutex.unlock();
}

const AVFrame *VideoBuffer::consumeRenderedFrame()
{
    Q_ASSERT(!m_renderingFrameConsumed);
    m_renderingFrameConsumed = true;
    if (m_fpsCounter.isStarted()) {
        m_fpsCounter.addRenderedFrame();
    }
    if (m_renderExpiredFrames) {
        // if m_renderExpiredFrames is enable, then notify the decoder the current frame is
        // consumed, so that it may push a new one
        m_renderingFrameConsumedCond.wakeOne();
    }
    return m_renderingframe;
}

void VideoBuffer::peekRenderedFrame(std::function<void(int width, int height, uint8_t* dataRGB32)> onFrame)
{
    if (!onFrame) {
        return;
    }

    lock();
    auto frame = m_renderingframe;
    int width = frame->width;
    int height = frame->height;

    // create buffer
    uint8_t* rgbBuffer = new uint8_t[width * height * 4];
    AVFrame *rgbFrame = av_frame_alloc();
    if (!rgbFrame) {
        delete [] rgbBuffer;
        return;
    }

    // bind buffer to AVFrame
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, rgbBuffer, AV_PIX_FMT_RGB32, width, height, 4);

    // convert
    AVFrameConvert convert;
    convert.setSrcFrameInfo(width, height, AV_PIX_FMT_YUV420P);
    convert.setDstFrameInfo(width, height, AV_PIX_FMT_RGB32);
    bool ret = false;
    ret = convert.init();
    if (!ret) {
        delete [] rgbBuffer;
        av_free(rgbFrame);
        return;
    }
    ret = convert.convert(frame, rgbFrame);
    if (!ret) {
        delete [] rgbBuffer;
        av_free(rgbFrame);
        return;
    }
    convert.deInit();
    av_free(rgbFrame);
    unLock();

    onFrame(width, height, rgbBuffer);
    delete [] rgbBuffer;
}

void VideoBuffer::interrupt()
{
    if (m_renderExpiredFrames) {
        m_mutex.lock();
        m_interrupted = true;
        m_mutex.unlock();
        // wake up blocking wait
        m_renderingFrameConsumedCond.wakeOne();
    }
}

void VideoBuffer::swap()
{
    AVFrame *tmp = m_decodingFrame;
    m_decodingFrame = m_renderingframe;
    m_renderingframe = tmp;
}
