#include "windowframelessmanager.h"
#include "nativewindowutils.h"
#include "windowframelesshelper.h"

#ifdef Q_OS_WIN32
#include "windownativeeventfilterwin.h"
#endif

static QVector<WindowFramelessHelper *> s_windowFramelessHelpers;

WindowFramelessManager::WindowFramelessManager()
{
#ifdef Q_OS_WIN32
    WindowNativeEventFilterWin::Instance()->Init();
#endif
}

WindowFramelessManager::~WindowFramelessManager() {}

WindowFramelessManager *WindowFramelessManager::Instance()
{
    static WindowFramelessManager windowNativeEventFilter;
    return &windowNativeEventFilter;
}

void WindowFramelessManager::addWindow(WindowFramelessHelper *win)
{
    if (nullptr == win) {
        return;
    }
    s_windowFramelessHelpers.push_back(win);
}

void WindowFramelessManager::removeWindow(WindowFramelessHelper *win)
{
    if (nullptr == win) {
        return;
    }
    s_windowFramelessHelpers.removeOne(win);
}

WindowFramelessHelper *WindowFramelessManager::getWindowByHandle(quint64 handle)
{
    quint64 targetHandle = 0;
    for (auto i = s_windowFramelessHelpers.begin(); i != s_windowFramelessHelpers.end(); i++) {
        if ((*i)->target() == nullptr) {
            continue;
        }

        targetHandle = NativeWindowUtils::GetHandleByWId((*i)->target()->winId());
        if (targetHandle == handle) {
            return (*i);
        }
    }

    return nullptr;
}
