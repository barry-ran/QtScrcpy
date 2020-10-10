#pragma once
#include <QWindow>

#if defined(Q_OS_WIN)
#include <QAbstractNativeEventFilter>
#include <QMargins>

class WindowFramelessHelper : public QAbstractNativeEventFilter
{
protected:
    WindowFramelessHelper();
    ~WindowFramelessHelper() override;

public:
    static WindowFramelessHelper *Instance();

    void Init();
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

private:
    bool processNcHitTest(void *message, long *result);
    bool processNcLButtonDown(void *message, long *result);
    bool processSetCursor(void *message, long *result);

    QWindow *getWindow(WId wndId);

private:
    bool m_inited = false;
    QMargins m_windowMargin = QMargins(0, 0, 0, 0);
};

#endif // Q_OS_WIN
