#include <QDebug>
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi")

#include "winutils.h"

enum : WORD
{
    DwmwaUseImmersiveDarkMode = 20,
    DwmwaUseImmersiveDarkModeBefore20h1 = 19
};

WinUtils::WinUtils(){};

WinUtils::~WinUtils(){};

// Set dark border to window
// Reference: qt/qtbase.git/tree/src/plugins/platforms/windows/qwindowswindow.cpp
bool WinUtils::setDarkBorderToWindow(const HWND &hwnd, const bool &d)
{
    const BOOL darkBorder = d ? TRUE : FALSE;
    const bool ok = SUCCEEDED(DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkMode, &darkBorder, sizeof(darkBorder)))
                    || SUCCEEDED(DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkModeBefore20h1, &darkBorder, sizeof(darkBorder)));
    if (!ok)
        qWarning("%s: Unable to set dark window border.", __FUNCTION__);
    return ok;
}
