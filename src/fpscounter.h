#ifndef FPSCOUNTER_H
#define FPSCOUNTER_H
#include <QTime>

class FpsCounter
{
public:
    FpsCounter();
    virtual ~FpsCounter();

    void fpsCounterInit();
    void fpsCounterStart();
    void fpsCounterStop();
    void fpsCounterAddRenderedFrame();
#ifdef SKIP_FRAMES
    void fpsCounterAddSkippedFrame();
#endif

private:
    void checkExpired();
    void displayFps();

private:
    bool m_started = false;
    QTime m_timeCounter;
    quint32 m_rendered = 0;
#ifdef SKIP_FRAMES
    quint32 m_skipped = 0;
#endif
};

#endif // FPSCOUNTER_H
