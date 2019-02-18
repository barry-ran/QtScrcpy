#import <Cocoa/Cocoa.h>
#include <QDebug>
#include <QWidget>

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
    QWidget* widget = Q_NULLPTR;
} MouseEventTapData;

static CGEventRef Cocoa_MouseTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
    Q_UNUSED(proxy);
    MouseEventTapData *tapdata = (MouseEventTapData*)refcon;

    NSView *nsview;
    NSWindow *nswindow;
    NSRect windowRect;
    NSRect newWindowRect;
    CGPoint eventLocation;

    switch (type) {
        case kCGEventTapDisabledByTimeout:
            {
                CGEventTapEnable(tapdata->tap, true);
                return NULL;
            }
        case kCGEventTapDisabledByUserInput:
            {
                return NULL;
            }
        default:
            break;
    }


    if (!tapdata->widget) {
        return event;
    }
    // get nswindow from qt widget
    nsview = (NSView *)tapdata->widget->window()->winId();
    if (!nsview) {
        return event;
    }
    nswindow = [nsview window];

    eventLocation = CGEventGetUnflippedLocation(event);
    windowRect = [nswindow contentRectForFrameRect:[nswindow frame]];

    newWindowRect = NSMakeRect(windowRect.origin.x, windowRect.origin.y,
                            windowRect.size.width - 10, windowRect.size.height - 10);
    qDebug() << newWindowRect.origin.x << newWindowRect.origin.y
             << newWindowRect.size.width << newWindowRect.size.height;

    if (!NSMouseInRect(NSPointFromCGPoint(eventLocation), newWindowRect, NO)) {

        /* This is in CGs global screenspace coordinate system, which has a
         * flipped Y.
         */
        CGPoint newLocation = CGEventGetLocation(event);

        if (eventLocation.x < NSMinX(windowRect)) {
            newLocation.x = NSMinX(windowRect);
        } else if (eventLocation.x >= NSMaxX(windowRect)) {
            newLocation.x = NSMaxX(windowRect) - 1.0;
        }

        if (eventLocation.y <= NSMinY(windowRect)) {
            newLocation.y -= (NSMinY(windowRect) - eventLocation.y + 1);
        } else if (eventLocation.y > NSMaxY(windowRect)) {
            newLocation.y += (eventLocation.y - NSMaxY(windowRect));
        }

        CGWarpMouseCursorPosition(newLocation);
        CGAssociateMouseAndMouseCursorPosition(YES);

        if ((CGEventMaskBit(type) & movementEventsMask) == 0) {
            /* For click events, we just constrain the event to the window, so
             * no other app receives the click event. We can't due the same to
             * movement events, since they mean that our warp cursor above
             * behaves strangely.
             */
            CGEventSetLocation(event, newLocation);
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

void CocoaMouseTap::enableMouseEventTap(QWidget* widget, bool enabled)
{
    if (m_tapData && m_tapData->tap)
    {
        enabled ? m_tapData->widget = widget : m_tapData->widget = Q_NULLPTR;
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
