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

void WinMouseTap::enableMouseEventTap(QWidget *widget, bool enabled)
{
    if (!widget) {
        return;
    }
    if(enabled) {
        QRect rc(widget->parentWidget()->mapToGlobal(widget->pos())
                 , widget->size());
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
