#include <QtGlobal>

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QtX11Extras/QX11Info>
#else
#include <QtGui/private/qtx11extras_p.h>
#endif

#include <xcb/xproto.h>
#include <stdlib.h>
#include <stdint.h>

#include "xmousetap.h"

XMouseTap::XMouseTap() {}

XMouseTap::~XMouseTap() {}

void XMouseTap::initMouseEventTap() {}

void XMouseTap::quitMouseEventTap() {}

static void find_grab_window_recursive(xcb_connection_t *dpy, xcb_window_t window,
        QRect rc, int16_t offset_x, int16_t offset_y,
        xcb_window_t *grab_window, uint32_t *grab_window_size) {
    xcb_query_tree_cookie_t tree_cookie;
    xcb_query_tree_reply_t *tree;
    tree_cookie = xcb_query_tree(dpy, window);
    tree = xcb_query_tree_reply(dpy, tree_cookie, NULL);

    xcb_window_t *children = xcb_query_tree_children(tree);
    for (int i = 0; i < xcb_query_tree_children_length(tree); i++) {
        xcb_get_geometry_cookie_t gg_cookie;
        xcb_get_geometry_reply_t *gg;
        gg_cookie = xcb_get_geometry(dpy, children[i]);
        gg = xcb_get_geometry_reply(dpy, gg_cookie, NULL);

        if (gg->x + offset_x <= rc.left() && gg->x + offset_x + gg->width >= rc.right() &&
                gg->y + offset_y <= rc.top() && gg->y + offset_y + gg->height >= rc.bottom()) {
            if (!*grab_window || gg->width * gg->height <= *grab_window_size) {
                *grab_window = children[i];
                *grab_window_size = gg->width * gg->height;
            }
        }

        find_grab_window_recursive(dpy, children[i], rc,
                gg->x + offset_x, gg->y + offset_y,
                grab_window, grab_window_size);

        free(gg);
    }

    free(tree);
}

void XMouseTap::enableMouseEventTap(QRect rc, bool enabled) {
    if (enabled && rc.isEmpty()) {
        return;
    }

    xcb_connection_t *dpy = QX11Info::connection();

    if (enabled) {
        // We grab the top-most smallest window
        xcb_window_t grab_window = 0;
        uint32_t grab_window_size = 0;

        find_grab_window_recursive(dpy, QX11Info::appRootWindow(QX11Info::appScreen()),
                rc, 0, 0, &grab_window, &grab_window_size);

        if (grab_window) {
            xcb_grab_pointer_cookie_t grab_cookie;
            xcb_grab_pointer_reply_t *grab;
            grab_cookie = xcb_grab_pointer(dpy, /* owner_events = */ 1,
                    grab_window, /* event_mask = */ 0,
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                    grab_window, XCB_NONE, XCB_CURRENT_TIME);
            grab = xcb_grab_pointer_reply(dpy, grab_cookie, NULL);

            free(grab);
        }
    } else {
        xcb_void_cookie_t ungrab_cookie;
        xcb_generic_error_t *error;
        ungrab_cookie = xcb_ungrab_pointer_checked(dpy, XCB_CURRENT_TIME);
        error = xcb_request_check(dpy, ungrab_cookie);

        free(error);
    }
}
