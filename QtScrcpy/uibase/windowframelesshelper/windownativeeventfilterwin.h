#pragma once
#include <QAbstractNativeEventFilter>
#include <QMargins>
#include <QWindow>

class WindowNativeEventFilterWin : public QAbstractNativeEventFilter
{
protected:
    WindowNativeEventFilterWin();
    ~WindowNativeEventFilterWin() override;

public:
    static WindowNativeEventFilterWin *Instance();

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
