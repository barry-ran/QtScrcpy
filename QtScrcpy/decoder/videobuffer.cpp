#include "videobuffer.h"
extern "C"
{
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
}

VideoBuffer::VideoBuffer()
{

}

VideoBuffer::~VideoBuffer()
{

}

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

AVFrame *VideoBuffer::decodingFrame()
{
    return m_decodingFrame;
}

void VideoBuffer::offerDecodedFrame(bool& previousFrameSkipped)
{
    m_mutex.lock();

#ifndef SKIP_FRAMES
    // if SKIP_FRAMES is disabled, then the decoder must wait for the current
    // frame to be consumed
    while (!m_renderingFrameConsumed && !m_interrupted) {
        m_renderingFrameConsumedCond.wait(&m_mutex);
    }    
#else
    if (m_fpsCounter.isStarted() && !m_renderingFrameConsumed) {
        m_fpsCounter.addSkippedFrame();
    }
#endif

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
#ifndef SKIP_FRAMES
    // if SKIP_FRAMES is disabled, then notify the decoder the current frame is
    // consumed, so that it may push a new one
    m_renderingFrameConsumedCond.wakeOne();
#endif
    return m_renderingframe;
}

void VideoBuffer::interrupt()
{
#ifndef SKIP_FRAMES
    m_mutex.lock();
    m_interrupted = true;
    m_mutex.unlock();
    // wake up blocking wait
    m_renderingFrameConsumedCond.wakeOne();
#endif
}

void VideoBuffer::swap()
{
    AVFrame *tmp = m_decodingFrame;
    m_decodingFrame = m_renderingframe;
    m_renderingframe = tmp;
}
