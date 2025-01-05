#include <xcb/xproto.h>
#include <stdlib.h>

#include <QDebug>
#include <QRect>
#include <xcb/xproto.h>
#include <QGuiApplication>
#include <QRect>

#include "xmousetap.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <xcb/xcb.h>

XMouseTap::XMouseTap() {}

XMouseTap::~XMouseTap() {}

void XMouseTap::initMouseEventTap() {}

void XMouseTap::quitMouseEventTap() {}

void XMouseTap::enableMouseEventTap(QRect rc, bool enabled) {
    if (enabled && rc.isEmpty()) {
        return;
    }

    auto *x11Interface = qApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!x11Interface) {
        qWarning() << "X11 interface is not available. Ensure the application is running on X11.";
        return;
    }

    Display *display = reinterpret_cast<Display*>(x11Interface->display());
    if (!display) {
        qWarning() << "Failed to get X Display.";
        return;
    }

    int screenNumber = DefaultScreen(display);

    Window rootWindow = RootWindow(display, screenNumber);

    XRectangle xRect;
    xRect.x = static_cast<short>(rc.x());
    xRect.y = static_cast<short>(rc.y());
    xRect.width = static_cast<unsigned short>(rc.width());
    xRect.height = static_cast<unsigned short>(rc.height());

    if (enabled) {
        int result = XGrabPointer(display, rootWindow, True,
                                  ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                                  GrabModeAsync, GrabModeAsync,
                                  None, None, CurrentTime);
        if (result != GrabSuccess) {
            qWarning() << "Failed to grab pointer.";
        }
    } else {
        XUngrabPointer(display, CurrentTime);
    }
    XFlush(display);
}
