#pragma once
#include <QWindow>

class NativeWindowUtils
{
public:
    static quint64 GetHandleByWId(WId id);
};
