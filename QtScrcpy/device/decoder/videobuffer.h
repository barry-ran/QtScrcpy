#ifndef VIDEO_BUFFER_H
#define VIDEO_BUFFER_H

#include <QMutex>
#include <QWaitCondition>

#include "fpscounter.h"

// forward declarations
typedef struct AVFrame AVFrame;

class VideoBuffer
{    
public:
    VideoBuffer();
    virtual ~VideoBuffer();

    bool init();
    void deInit();
    void lock();
    void unLock();

    AVFrame* decodingFrame();
    // set the decoder frame as ready for rendering
    // this function locks m_mutex during its execution
    // returns true if the previous frame had been consumed
    void offerDecodedFrame(bool& previousFrameSkipped);

    // mark the rendering frame as consumed and return it
    // MUST be called with m_mutex locked!!!
    // the caller is expected to render the returned frame to some texture before
    // unlocking m_mutex
    const AVFrame* consumeRenderedFrame();

    // wake up and avoid any blocking call
    void interrupt();

private:
    void swap();

private:
    AVFrame* m_decodingFrame = Q_NULLPTR;
    AVFrame* m_renderingframe = Q_NULLPTR;
    QMutex m_mutex;
    bool m_renderingFrameConsumed = true;
    FpsCounter m_fpsCounter;

#ifndef SKIP_FRAMES
    QWaitCondition m_renderingFrameConsumedCond;
    bool m_interrupted = true;
#endif
};

#endif // VIDEO_BUFFER_H
