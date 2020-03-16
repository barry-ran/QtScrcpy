#include <Windows.h>
#include <QWidget>
#include <QDebug>

#include "winmousetap.h"

WinMouseTap::WinMouseTap()
{

}

WinMouseTap::~WinMouseTap()
{

}

void WinMouseTap::initMouseEventTap()
{

}

void WinMouseTap::quitMouseEventTap()
{

}

void WinMouseTap::enableMouseEventTap(QRect rc, bool enabled)
{
    if (enabled && rc.isEmpty()) {
        return;
    }
    if(enabled) {
        RECT mainRect;
        mainRect.left = (LONG)rc.left();
        mainRect.right = (LONG)rc.right();
        mainRect.top = (LONG)rc.top();
        mainRect.bottom = (LONG)rc.bottom();
        ClipCursor(&mainRect);
    } else {
        ClipCursor(Q_NULLPTR);
    }
}
