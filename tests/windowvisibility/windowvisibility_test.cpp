#include "windowvisibilitycontroller.h"

#include <QApplication>
#include <QCursor>
#include <QMenu>
#include <QScreen>
#include <QSignalSpy>
#include <QtTest>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {
bool opacityIs(QWidget &window, qreal opacity)
{
    return qAbs(window.windowOpacity() - opacity) <= 1.0 / 255.0;
}

class TestWindows
{
public:
    QWidget host;
    QWidget tool;
    WindowVisibilityController controller;

    TestWindows() : tool(nullptr, Qt::Tool), controller(&host)
    {
        const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
        host.setGeometry(screen.x() + 80, screen.y() + 100, 220, 180);
        tool.setGeometry(screen.x() + 480, screen.y() + 100, 90, 180);
        host.show();
        tool.show();
        controller.addWindow(&tool);
    }

    void moveAway()
    {
        QCursor::setPos(QGuiApplication::primaryScreen()->availableGeometry().bottomRight() - QPoint(5, 5));
    }
};
}

class WindowVisibilityTest : public QObject
{
    Q_OBJECT

private:
    QPoint m_originalCursor;

private slots:
    void init()
    {
        m_originalCursor = QCursor::pos();
        if (QGuiApplication::platformName() == QStringLiteral("offscreen")
            || QGuiApplication::platformName() == QStringLiteral("minimal"))
            QSKIP("Requires desktop windows for native pointer hit testing.");
    }

    void cleanup()
    {
        QCursor::setPos(m_originalCursor);
    }

    void defaultsManualAndNewWindows()
    {
        QWidget host;
        QWidget tool(nullptr, Qt::Tool);
        WindowVisibilityController controller(&host);
        QCOMPARE(controller.opacityPercent(), 100);
        QVERIFY(!controller.autoHideEnabled());
        QSignalSpy opacitySpy(&controller, &WindowVisibilityController::opacityPercentChanged);
        controller.setOpacityPercent(55);
        controller.addWindow(&tool);
        QVERIFY(opacityIs(host, 0.55));
        QVERIFY(opacityIs(tool, 0.55));
        QCOMPARE(opacitySpy.count(), 1);
        controller.setOpacityPercent(55);
        QCOMPARE(opacitySpy.count(), 1);
        controller.setOpacityPercent(0);
        QCOMPARE(controller.opacityPercent(), 10);
        controller.setOpacityPercent(101);
        QCOMPARE(controller.opacityPercent(), 100);
    }

    void leaveDelayRevealDelayAndDisableRestore()
    {
        TestWindows windows;
        QVERIFY(QTest::qWaitForWindowExposed(&windows.host));
        windows.controller.setOpacityPercent(65);
        windows.moveAway();
        QElapsedTimer delay;
        delay.start();
        windows.controller.setAutoHideEnabled(true);
        QTest::qWait(75);
        // Desktop composition can block the test event loop. Assert the
        // intermediate state only while genuinely inside the delay interval.
        if (delay.elapsed() < 250)
            QVERIFY(opacityIs(windows.host, 0.65));
        QTRY_VERIFY_WITH_TIMEOUT(windows.host.windowOpacity() < 0.01, 1500);
        QVERIFY(windows.tool.windowOpacity() < 0.01);
        windows.controller.setOpacityPercent(45);
        QVERIFY(windows.host.windowOpacity() < 0.01);
        delay.restart();
        QCursor::setPos(windows.tool.frameGeometry().center());
        QTest::qWait(75);
        if (delay.elapsed() < 250)
            QVERIFY(windows.host.windowOpacity() < 0.01);
        QTRY_VERIFY_WITH_TIMEOUT(opacityIs(windows.host, 0.45), 1500);
        QVERIFY(opacityIs(windows.tool, 0.45));
        windows.moveAway();
        QTRY_VERIFY_WITH_TIMEOUT(windows.host.windowOpacity() < 0.01, 1500);
        windows.controller.setAutoHideEnabled(false);
        QVERIFY(opacityIs(windows.host, 0.45));
        QVERIFY(opacityIs(windows.tool, 0.45));
    }

    void distantGapIsNotPartOfGroup()
    {
        TestWindows windows;
        QVERIFY(QTest::qWaitForWindowExposed(&windows.tool));
        QCursor::setPos(QPoint((windows.host.frameGeometry().right() + windows.tool.frameGeometry().left()) / 2,
                              windows.host.frameGeometry().center().y()));
        windows.controller.setAutoHideEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(windows.host.windowOpacity() < 0.01, 1500);
    }

    void coveringWindowCannotReveal()
    {
        TestWindows windows;
        QVERIFY(QTest::qWaitForWindowExposed(&windows.host));
        windows.moveAway();
        windows.controller.setAutoHideEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(windows.host.windowOpacity() < 0.01, 1500);
        QWidget cover(nullptr, Qt::Window | Qt::WindowStaysOnTopHint);
        cover.setGeometry(windows.host.geometry());
        cover.show();
        cover.raise();
        QVERIFY(QTest::qWaitForWindowExposed(&cover));
        QCursor::setPos(windows.host.frameGeometry().center());
        QTest::qWait(550);
        QVERIFY(windows.host.windowOpacity() < 0.01);
        cover.hide();
        QTRY_VERIFY_WITH_TIMEOUT(opacityIs(windows.host, 1.0), 1500);
    }

    void outsideMarginOnlyKeepsVisibleState()
    {
        TestWindows windows;
        QVERIFY(QTest::qWaitForWindowExposed(&windows.host));
        const QRect frame = windows.host.frameGeometry();
        QPoint marginPoint(frame.left() - 6, frame.center().y());
#ifdef Q_OS_WIN
        // Windows invisible resize borders can extend beyond Qt's frame.
        // Choose a point outside the actual native rectangle, as the
        // controller does, and retain its Qt logical position for reuse.
        RECT nativeFrame = {};
        QVERIFY(GetWindowRect(reinterpret_cast<HWND>(windows.host.winId()), &nativeFrame));
        QVERIFY(SetCursorPos(nativeFrame.left - qRound(6 * windows.host.devicePixelRatioF()),
                             (nativeFrame.top + nativeFrame.bottom) / 2));
        marginPoint = QCursor::pos();
#endif
        QCursor::setPos(marginPoint);
        windows.controller.setAutoHideEnabled(true);
        QTest::qWait(500);
        QVERIFY(opacityIs(windows.host, 1.0));
        windows.moveAway();
        QTRY_VERIFY_WITH_TIMEOUT(windows.host.windowOpacity() < 0.01, 1500);
        QCursor::setPos(marginPoint);
        QTest::qWait(500);
        QVERIFY(windows.host.windowOpacity() < 0.01);
    }

    void mouseCaptureKeepsGroupVisible()
    {
        TestWindows windows;
        QVERIFY(QTest::qWaitForWindowExposed(&windows.host));
        windows.host.grabMouse();
        windows.moveAway();
        windows.controller.setAutoHideEnabled(true);
        QTest::qWait(500);
        QVERIFY(opacityIs(windows.host, 1.0));
        windows.host.releaseMouse();
        QTRY_VERIFY_WITH_TIMEOUT(windows.host.windowOpacity() < 0.01, 1500);
    }

    void toolbarReshowCannotExposeHiddenGroup()
    {
        TestWindows windows;
        QVERIFY(QTest::qWaitForWindowExposed(&windows.host));
        windows.moveAway();
        windows.controller.setAutoHideEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(windows.host.windowOpacity() < 0.01, 1500);
        windows.tool.hide();
        windows.tool.setWindowFlag(Qt::WindowStaysOnTopHint, true);
        windows.tool.show();
        QVERIFY(windows.tool.windowOpacity() < 0.01);
        QTest::qWait(100);
        QVERIFY(windows.host.windowOpacity() < 0.01);
        QVERIFY(windows.tool.windowOpacity() < 0.01);
    }

    void popupOperationAndHostPauseRestore()
    {
        TestWindows windows;
        QVERIFY(QTest::qWaitForWindowExposed(&windows.host));
        windows.controller.setOpacityPercent(75);
        QMenu menu(&windows.tool);
        menu.addAction(QStringLiteral("Test action"));
        menu.popup(windows.tool.frameGeometry().bottomLeft());
        QVERIFY(QTest::qWaitForWindowExposed(&menu));
        windows.moveAway();
        windows.controller.setAutoHideEnabled(true);
        QTest::qWait(600);
        QVERIFY(opacityIs(windows.host, 0.75));
        QVERIFY(opacityIs(menu, 0.75));
        menu.hide();
        QTRY_VERIFY_WITH_TIMEOUT(windows.host.windowOpacity() < 0.01, 1500);
        windows.host.hide();
        QVERIFY(opacityIs(windows.host, 0.75));
        QVERIFY(opacityIs(windows.tool, 0.75));
        windows.host.show();
        QTest::qWait(150);
        QVERIFY(opacityIs(windows.host, 0.75));
    }

    void controllerRemovalRestoresWindows()
    {
        QWidget host;
        QWidget tool(nullptr, Qt::Tool);
        auto *controller = new WindowVisibilityController(&host);
        controller->addWindow(&tool);
        controller->setOpacityPercent(20);
        QVERIFY(opacityIs(host, 0.20));
        delete controller;
        QVERIFY(opacityIs(host, 1.0));
        QVERIFY(opacityIs(tool, 1.0));
    }
};

QTEST_MAIN(WindowVisibilityTest)
#include "windowvisibility_test.moc"
