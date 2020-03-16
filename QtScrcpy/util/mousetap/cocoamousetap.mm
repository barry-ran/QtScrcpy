#import <Cocoa/Cocoa.h>
#include <QDebug>

#include "cocoamousetap.h"

static const CGEventMask movementEventsMask =
      CGEventMaskBit(kCGEventLeftMouseDragged)
    | CGEventMaskBit(kCGEventRightMouseDragged)
    | CGEventMaskBit(kCGEventMouseMoved);

static const CGEventMask allGrabbedEventsMask =
      CGEventMaskBit(kCGEventLeftMouseDown)    | CGEventMaskBit(kCGEventLeftMouseUp)
    | CGEventMaskBit(kCGEventRightMouseDown)   | CGEventMaskBit(kCGEventRightMouseUp)
    | CGEventMaskBit(kCGEventOtherMouseDown)   | CGEventMaskBit(kCGEventOtherMouseUp)
    | CGEventMaskBit(kCGEventLeftMouseDragged) | CGEventMaskBit(kCGEventRightMouseDragged)
    | CGEventMaskBit(kCGEventMouseMoved);

typedef struct MouseEventTapData{
    CFMachPortRef tap = Q_NULLPTR;
    CFRunLoopRef runloop = Q_NULLPTR;
    CFRunLoopSourceRef runloopSource = Q_NULLPTR;
    QRect rc;
} MouseEventTapData;

static CGEventRef Cocoa_MouseTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
    Q_UNUSED(proxy);
    MouseEventTapData *tapdata = (MouseEventTapData*)refcon;
    switch (type) {
        case kCGEventTapDisabledByTimeout:
            {
                CGEventTapEnable(tapdata->tap, true);
                return nullptr;
            }
        case kCGEventTapDisabledByUserInput:
            {
                return nullptr;
            }
        default:
            break;
    }


    if (tapdata->rc.isEmpty()) {
        return event;
    }

    NSRect limitWindowRect = NSMakeRect(tapdata->rc.left(), tapdata->rc.top(),
                                   tapdata->rc.width(), tapdata->rc.height());
    // check rect samll than limit rect
    NSRect checkWindowRect = NSMakeRect(limitWindowRect.origin.x + 10, limitWindowRect.origin.y + 10,
                            limitWindowRect.size.width - 10, limitWindowRect.size.height - 10);
    /* This is in CGs global screenspace coordinate system, which has a
     * flipped Y.
     */
    CGPoint eventLocation = CGEventGetLocation(event);
    if (!NSMouseInRect(NSPointFromCGPoint(eventLocation), checkWindowRect, NO)) {
        if (eventLocation.x <= NSMinX(limitWindowRect)) {
            eventLocation.x = NSMinX(limitWindowRect) + 1.0;
        } else if (eventLocation.x >= NSMaxX(limitWindowRect)) {
            eventLocation.x = NSMaxX(limitWindowRect) - 1.0;
        }

        if (eventLocation.y <= NSMinY(limitWindowRect)) {
            eventLocation.y = NSMinY(limitWindowRect) + 1.0;
        } else if (eventLocation.y >= NSMaxY(limitWindowRect)) {
            eventLocation.y = NSMaxY(limitWindowRect) - 1.0;
        }

        CGWarpMouseCursorPosition(eventLocation);
        CGAssociateMouseAndMouseCursorPosition(YES);

        if ((CGEventMaskBit(type) & movementEventsMask) == 0) {
            /* For click events, we just constrain the event to the window, so
             * no other app receives the click event. We can't due the same to
             * movement events, since they mean that our warp cursor above
             * behaves strangely.
             */
            CGEventSetLocation(event, eventLocation);
        }
    }

    return event;
}

static void SemaphorePostCallback(CFRunLoopTimerRef timer, void *info)
{
    Q_UNUSED(timer);
    QSemaphore *runloopStartedSemaphore = (QSemaphore *)info;
    if (runloopStartedSemaphore) {
        runloopStartedSemaphore->release();
    }
}

CocoaMouseTap::CocoaMouseTap(QObject *parent)
    : QThread(parent)
{
    m_tapData = new MouseEventTapData;
}

CocoaMouseTap::~CocoaMouseTap()
{
    if (m_tapData) {
        delete m_tapData;
        m_tapData = Q_NULLPTR;
    }
}

void CocoaMouseTap::initMouseEventTap()
{
    if (!m_tapData) {
        return;
    }

    m_tapData->tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                                      kCGEventTapOptionDefault, allGrabbedEventsMask,
                                      &Cocoa_MouseTapCallback, m_tapData);
    if (!m_tapData->tap) {
        return;
    }
    /* Tap starts disabled, until app requests mouse grab */
    CGEventTapEnable(m_tapData->tap, false);
    start();
}

void CocoaMouseTap::quitMouseEventTap()
{
    bool status;
    if (m_tapData == Q_NULLPTR || m_tapData->tap == Q_NULLPTR) {
        /* event tap was already cleaned up (possibly due to CGEventTapCreate
         * returning null.)
         */
        return;
    }

    /* Ensure that the runloop has been started first.
     * TODO: Move this to InitMouseEventTap, check for error conditions that can
     * happen in Cocoa_MouseTapThread, and fall back to the non-EventTap way of
     * grabbing the mouse if it fails to Init.
     */
    status = m_runloopStartedSemaphore.tryAcquire(1, 5000);
    if (status) {
        /* Then stop it, which will cause Cocoa_MouseTapThread to return. */
        CFRunLoopStop(m_tapData->runloop);
        /* And then wait for Cocoa_MouseTapThread to finish cleaning up. It
         * releases some of the pointers in tapdata. */
        wait();
    }
}

void CocoaMouseTap::enableMouseEventTap(QRect rc, bool enabled)
{
    if (m_tapData && m_tapData->tap)
    {
        enabled ? m_tapData->rc = rc : m_tapData->rc = QRect();
        CGEventTapEnable(m_tapData->tap, enabled);
    }
}

void CocoaMouseTap::run()
{
    /* Tap was created on main thread but we own it now. */
    CFMachPortRef eventTap = m_tapData->tap;
    if (eventTap) {
        /* Try to create a runloop source we can schedule. */
        CFRunLoopSourceRef runloopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
        if  (runloopSource) {
            m_tapData->runloopSource = runloopSource;
        } else {
            CFRelease(eventTap);
            m_runloopStartedSemaphore.release();
            /* TODO: Both here and in the return below, set some state in
             * tapdata to indicate that initialization failed, which we should
             * check in InitMouseEventTap, after we move the semaphore check
             * from Quit to Init.
             */
            return;
        }
    } else {
        m_runloopStartedSemaphore.release();
        return;
    }

    m_tapData->runloop = CFRunLoopGetCurrent();
    CFRunLoopAddSource(m_tapData->runloop, m_tapData->runloopSource, kCFRunLoopCommonModes);
    CFRunLoopTimerContext context = {.info = &m_runloopStartedSemaphore};
    /* We signal the runloop started semaphore *after* the run loop has started, indicating it's safe to CFRunLoopStop it. */
    CFRunLoopTimerRef timer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent(), 0, 0, 0, &SemaphorePostCallback, &context);
    CFRunLoopAddTimer(m_tapData->runloop, timer, kCFRunLoopCommonModes);
    CFRelease(timer);

    /* Run the event loop to handle events in the event tap. */
    CFRunLoopRun();
    /* Make sure this is signaled so that SDL_QuitMouseEventTap knows it can safely SDL_WaitThread for us. */
    if (m_runloopStartedSemaphore.available() < 1) {
        m_runloopStartedSemaphore.release();
    }
    CFRunLoopRemoveSource(m_tapData->runloop, m_tapData->runloopSource, kCFRunLoopCommonModes);

    /* Clean up. */
    CGEventTapEnable(m_tapData->tap, false);
    CFRelease(m_tapData->runloopSource);
    CFRelease(m_tapData->tap);
    m_tapData->runloopSource = Q_NULLPTR;
    m_tapData->tap = Q_NULLPTR;

    return;
}
