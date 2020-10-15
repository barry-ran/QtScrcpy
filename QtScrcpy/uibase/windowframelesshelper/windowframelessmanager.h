#pragma once
#include <QQuickWindow>
#include <QVector>

class WindowFramelessHelper;
class WindowFramelessManager
{
private:
    WindowFramelessManager();

public:
    virtual ~WindowFramelessManager();

public:
    static WindowFramelessManager* Instance();

    void addWindow(WindowFramelessHelper* win);
    void removeWindow(WindowFramelessHelper* win);
    WindowFramelessHelper* getWindowByHandle(quint64 handle);
};
