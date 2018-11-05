#ifndef FRAMES_H
#define FRAMES_H

#include <QMutex>
#include <QWaitCondition>

#include "fpscounter.h"

// forward declarations
typedef struct AVFrame AVFrame;

class Frames
{
public:
    Frames();
    virtual ~Frames();

    bool init();
    void deInit();
    void lock();
    void unLock();

    AVFrame* decodingFrame();
    // set the decoder frame as ready for rendering
    // this function locks m_mutex during its execution
    // returns true if the previous frame had been consumed
    bool offerDecodedFrame();

    // mark the rendering frame as consumed and return it
    // MUST be called with m_mutex locked!!!
    // the caller is expected to render the returned frame to some texture before
    // unlocking m_mutex
    const AVFrame* consumeRenderedFrame();

    // wake up and avoid any blocking call
    void stop();

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
    bool m_stopped = true;    
#endif
};

#endif // FRAMES_H
