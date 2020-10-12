#include "windowframelesshelper.h"

#include <Cocoa/Cocoa.h>
#include <QOperatingSystemVersion>

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

    //如果当前osx版本老于10.9，则后续代码不可用。转为使用定制的系统按钮，不支持自由缩放窗口及窗口阴影
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::OSXYosemite) {
        return;
    }

    NSView* view = (NSView*)target->winId();
    if (nullptr == view) {
        return;
    }

    NSWindow *window = view.window;
    if (nullptr == window) {
        return;
    }

    //设置标题文字和图标为不可见
    window.titleVisibility = NSWindowTitleHidden;
    //设置标题栏为透明
    window.titlebarAppearsTransparent = YES;
    //设置不可由标题栏拖动,避免与自定义拖动冲突
    window.movable = NO;
    window.hasShadow = YES;
    //设置view扩展到标题栏
    window.styleMask |=  NSWindowStyleMaskFullSizeContentView;

    emit targetChanged();
}
