#include "windowframelesshelper.h"
#include "windownativeeventfilter.h"

WindowFramelessHelper::WindowFramelessHelper(QObject *parent) : QObject(parent)
{

}

QQuickWindow *WindowFramelessHelper::target() const
{
    return m_target;
}

void WindowFramelessHelper::setTarget(QQuickWindow *target)
{
    if (target == m_target) {
        return;
    }
    m_target = target;

#ifdef Q_OS_WIN32
    WindowNativeEventFilter::Instance()->Init();
#endif

    emit targetChanged();
}

void WindowFramelessHelper::updateStyle() {

}
