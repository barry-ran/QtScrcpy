#ifndef WINUTILS_H
#define WINUTILS_H

#include <QApplication>
#include <Windows.h>

class WinUtils
{
public:
    WinUtils();
    ~WinUtils();

    static bool setDarkBorderToWindow(const HWND &hwnd, const bool &d);
};

#endif // WINUTILS_H
