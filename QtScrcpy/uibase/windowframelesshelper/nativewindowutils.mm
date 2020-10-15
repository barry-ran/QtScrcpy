#include "nativewindowutils.h"
#include <Cocoa/Cocoa.h>

quint64 NativeWindowUtils::GetHandleByWId(WId id) {
    NSView* view = (NSView*)id;
    if (nullptr == view) {
        return 0;
    }

    NSWindow *window = view.window;
    if (nullptr == window) {
        return 0;
    }
    return (quint64)window;
}
