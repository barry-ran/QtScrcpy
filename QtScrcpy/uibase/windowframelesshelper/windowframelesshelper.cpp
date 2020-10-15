#include "windowframelesshelper.h"
#include "windowframelessmanager.h"

WindowFramelessHelper::WindowFramelessHelper(QObject *parent) : QObject(parent)
{
    WindowFramelessManager::Instance()->addWindow(this);
}

WindowFramelessHelper::~WindowFramelessHelper()
{
    WindowFramelessManager::Instance()->removeWindow(this);
}

QQuickWindow *WindowFramelessHelper::target() const
{
    return m_target;
}

void WindowFramelessHelper::setTarget(QQuickWindow *target)
{
    if (target == nullptr || target == m_target) {
        return;
    }
    m_target = target;

    emit targetChanged();
}
