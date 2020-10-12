#include "windowframelesshelper.h"

#include <Cocoa/Cocoa.h>
#include <QOperatingSystemVersion>
#import  <objc/runtime.h>
#include <QDebug>

void SwizzleSelector(Class originalCls, SEL originalSelector, Class swizzledCls, SEL swizzledSelector) {
    Method originalMethod = class_getInstanceMethod(originalCls, originalSelector);
    Method swizzledMethod = class_getInstanceMethod(swizzledCls, swizzledSelector);

    BOOL didAddMethod =
    class_addMethod(originalCls,
                    originalSelector,
                    method_getImplementation(swizzledMethod),
                    method_getTypeEncoding(swizzledMethod));

    if (didAddMethod) {
        class_replaceMethod(originalCls,
                            swizzledSelector,
                            method_getImplementation(originalMethod),
                            method_getTypeEncoding(originalMethod));
    } else {
        method_exchangeImplementations(originalMethod, swizzledMethod);
    }
}


@interface NSWindow(FilterWindow)
@end
@implementation NSWindow(FilterWindow)
+ (void)load
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        SwizzleSelector([self class],
                          @selector(setStyleMask:),
                          [self class],
                          @selector(filter_setStyleMask:));
        SwizzleSelector([self class],
                          @selector(setTitlebarAppearsTransparent:),
                          [self class],
                          @selector(filter_setTitlebarAppearsTransparent:));
    });
}

- (void)filter_setStyleMask:(NSWindowStyleMask)styleMask
{
    [self filter_setStyleMask:styleMask];

    //设置标题文字和图标为不可见
    self.titleVisibility = NSWindowTitleHidden;
    //设置标题栏为透明
    self.titlebarAppearsTransparent = YES;
    //设置不可由标题栏拖动,避免与自定义拖动冲突
    self.movable = NO;
    self.hasShadow = YES;
    //设置view扩展到标题栏
    if (!(self.styleMask & NSWindowStyleMaskFullSizeContentView)) {
        self.styleMask |=  NSWindowStyleMaskFullSizeContentView;
    }
}

- (void)filter_setTitlebarAppearsTransparent:(BOOL)titlebarAppearsTransparent
{
    [self filter_setTitlebarAppearsTransparent:titlebarAppearsTransparent];
}
@end

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

    //updateStyle();

    emit targetChanged();
}
/*
void WindowFramelessHelper::updateStyle() {
    if (!m_target) {
        return;
    }

    //如果当前osx版本老于10.9，则后续代码不可用。转为使用定制的系统按钮，不支持自由缩放窗口及窗口阴影
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::OSXYosemite) {
        return;
    }

    NSView* view = (NSView*)m_target->winId();
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
}

*/
