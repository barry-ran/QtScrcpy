#include "windowframelessmanager.h"
#include "windowframelesshelper.h"
#include "nativewindowutils.h"

static QVector<WindowFramelessHelper*> s_windowFramelessHelpers;

WindowFramelessManager::WindowFramelessManager()
{

}

WindowFramelessManager::~WindowFramelessManager()
{

}

WindowFramelessManager *WindowFramelessManager::Instance()
{
    static WindowFramelessManager windowNativeEventFilter;
    return &windowNativeEventFilter;
}

void WindowFramelessManager::addWindow(WindowFramelessHelper* win)
{
    if (nullptr == win) {
        return;
    }
    s_windowFramelessHelpers.push_back(win);
}

void WindowFramelessManager::removeWindow(WindowFramelessHelper* win)
{
    if (nullptr == win) {
        return;
    }
    s_windowFramelessHelpers.removeOne(win);
}

WindowFramelessHelper* WindowFramelessManager::getWindowByHandle(quint64 handle)
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
