#include <QDebug>

#include "fpscounter.h"

FpsCounter::FpsCounter()
{

}

FpsCounter::~FpsCounter()
{

}

void FpsCounter::fpsCounterInit()
{
    m_started = false;
    // no need to initialize the other fields, they are meaningful only when
    // started is true
}

void FpsCounter::fpsCounterStart()
{
    m_started = true;
    m_timeCounter.start();
    m_rendered = 0;
#ifdef SKIP_FRAMES
    m_skipped = 0;
#endif
}

void FpsCounter::fpsCounterStop()
{
    m_started = false;
}

void FpsCounter::fpsCounterAddRenderedFrame()
{
    checkExpired();
    m_rendered++;
}

void FpsCounter::checkExpired()
{
    if (m_timeCounter.elapsed() >= 1000) {
        displayFps();
        m_timeCounter.restart();
        m_rendered = 0;
#ifdef SKIP_FRAMES
        m_skipped = 0;
#endif
       }
}

void FpsCounter::displayFps()
{
#ifdef SKIP_FRAMES
    if (m_skipped) {
        //qInfo << "%d fps (+%d frames skipped)", m_rendered, m_skipped);
    } else {
#endif
    //qInfo m_rendered << "fps";
#ifdef SKIP_FRAMES
    }
#endif
}

#ifdef SKIP_FRAMES
void FpsCounter::fpsCounterAddSkippedFrame()
{
    checkExpired();
    m_skipped++;
}
#endif
