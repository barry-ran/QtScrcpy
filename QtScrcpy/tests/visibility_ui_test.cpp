#include <QApplication>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QSignalSpy>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

#include "config.h"
#include "qyuvopenglwidget.h"
#include "toolform.h"
#include "videoform.h"

namespace {
const int FrameWidth = 360;
const int FrameHeight = 640;

// This background is deliberately a different color from the synthetic video.
// Screen captures therefore show the compositor's actual window transparency.
class CompositionBackdrop : public QWidget
{
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), QColor("#235577"));
        painter.setPen(QColor("#326987"));
        for (int x = 0; x < width(); x += 40)
            painter.drawLine(x, 0, x, height());
        for (int y = 0; y < height(); y += 40)
            painter.drawLine(0, y, width(), y);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Segoe UI", 14));
        painter.drawText(QPoint(24, 30), "QtScrcpy / OpenGL window composition test");
    }
};

struct SyntheticYuv
{
    QByteArray y = QByteArray(FrameWidth * FrameHeight, char(0));
    QByteArray u = QByteArray(FrameWidth * FrameHeight / 4, char(0));
    QByteArray v = QByteArray(FrameWidth * FrameHeight / 4, char(0));

    SyntheticYuv()
    {
        QImage card(FrameWidth, FrameHeight, QImage::Format_RGB32);
        card.fill(QColor("#f5ecd8"));
        QPainter painter(&card);
        painter.fillRect(QRect(0, 0, FrameWidth, 120), QColor("#2353a4"));
        painter.fillRect(QRect(0, 480, FrameWidth, 160), QColor("#a5452b"));
        painter.setPen(Qt::white);
        painter.setFont(QFont("Segoe UI", 23, QFont::Bold));
        painter.drawText(QRect(24, 24, FrameWidth - 48, 55), "QtScrcpy");
        painter.setFont(QFont("Segoe UI", 10));
        painter.drawText(QRect(24, 80, FrameWidth - 48, 30), "SYNTHETIC YUV VIDEO FRAME");
        painter.setPen(QColor("#342f29"));
        painter.setFont(QFont("Microsoft YaHei", 19));
        painter.drawText(QRect(26, 165, FrameWidth - 52, 55), QStringLiteral("手动调节透明度"));
        painter.drawText(QRect(26, 235, FrameWidth - 52, 55), QStringLiteral("鼠标离开自动隐身"));
        painter.setFont(QFont("Segoe UI", 12));
        painter.drawText(QRect(26, 340, FrameWidth - 52, 90), Qt::TextWordWrap,
                         "Real VideoForm + ToolForm\nNo phone connection required");
        painter.setPen(Qt::white);
        painter.drawText(QRect(26, 515, FrameWidth - 52, 75), Qt::TextWordWrap,
                         "100% opaque\n40% opacity / 60% transparent");
        painter.end();

        // Limited-range BT.709, matching the production OpenGL shader.
        for (int row = 0; row < FrameHeight; ++row) {
            for (int col = 0; col < FrameWidth; ++col) {
                const QColor color = card.pixelColor(col, row);
                y[row * FrameWidth + col] = char(qBound(0, qRound(16.0 + 0.182586 * color.red()
                                                                 + 0.614231 * color.green()
                                                                 + 0.062007 * color.blue()), 255));
            }
        }
        for (int row = 0; row < FrameHeight; row += 2) {
            for (int col = 0; col < FrameWidth; col += 2) {
                int red = 0;
                int green = 0;
                int blue = 0;
                for (int dy = 0; dy < 2; ++dy) {
                    for (int dx = 0; dx < 2; ++dx) {
                        const QColor color = card.pixelColor(col + dx, row + dy);
                        red += color.red();
                        green += color.green();
                        blue += color.blue();
                    }
                }
                const int index = (row / 2) * (FrameWidth / 2) + col / 2;
                u[index] = char(qBound(0, qRound(128.0 - 0.100644 * red / 4.0
                                                     - 0.338572 * green / 4.0
                                                     + 0.439216 * blue / 4.0), 255));
                v[index] = char(qBound(0, qRound(128.0 + 0.439216 * red / 4.0
                                                     - 0.398942 * green / 4.0
                                                     - 0.040274 * blue / 4.0), 255));
            }
        }
    }

    void present(VideoForm &video)
    {
        video.updateRender(FrameWidth, FrameHeight,
                           reinterpret_cast<uint8_t *>(y.data()),
                           reinterpret_cast<uint8_t *>(u.data()),
                           reinterpret_cast<uint8_t *>(v.data()),
                           FrameWidth, FrameWidth / 2, FrameWidth / 2);
    }
};

bool approximately(qreal actual, qreal expected)
{
    // QWidget reports the native 8-bit alpha value on Windows.
    return qAbs(actual - expected) <= 1.1 / 255.0;
}

int colorDifference(const QColor &first, const QColor &second)
{
    return qAbs(first.red() - second.red()) + qAbs(first.green() - second.green())
        + qAbs(first.blue() - second.blue());
}

QColor screenPixel(const QPoint &position)
{
    const QImage capture = QGuiApplication::primaryScreen()->grabWindow(0, position.x(), position.y(), 1, 1).toImage();
    return capture.isNull() ? QColor() : capture.pixelColor(0, 0);
}

bool saveScreenshot(VideoForm &video, ToolForm &toolbar, const QString &tag, bool primary = false)
{
    const QString requested = QString::fromLocal8Bit(qgetenv("QTSCRCPY_TEST_SCREENSHOT_PATH"));
    if (requested.isEmpty())
        return true;
    const QFileInfo info(requested);
    QDir().mkpath(info.absolutePath());
    const QString path = primary ? requested
        : info.dir().filePath(info.completeBaseName() + "-" + tag + ".png");
    QRect capture = video.frameGeometry().united(toolbar.frameGeometry());
    for (QMenu *menu : toolbar.findChildren<QMenu *>()) {
        if (menu->isVisible())
            capture = capture.united(menu->frameGeometry());
    }
    capture = capture.adjusted(-24, -24, 24, 24).intersected(QGuiApplication::primaryScreen()->geometry());
    const QPixmap pixmap = QGuiApplication::primaryScreen()->grabWindow(0, capture.x(), capture.y(), capture.width(), capture.height());
    const bool saved = !pixmap.isNull() && pixmap.save(path);
    qInfo().noquote() << "Compositor screenshot:" << path;
    return saved;
}

void positionVideo(VideoForm &video)
{
    const QRect area = QGuiApplication::primaryScreen()->availableGeometry();
    video.move(area.left() + qMin(100, area.width() / 8), area.top() + 70);
    video.raise();
    video.activateWindow();
}
}

class VisibilityUiTest : public QObject
{
    Q_OBJECT

private slots:
    void titledWindowControlsAndPersistence();
    void skinnedOpenGLComposition();
};

void VisibilityUiTest::titledWindowControlsAndPersistence()
{
    CompositionBackdrop backdrop;
    backdrop.setGeometry(QGuiApplication::primaryScreen()->availableGeometry().adjusted(12, 12, -12, -12));
    backdrop.show();
    QVERIFY(QTest::qWaitForWindowExposed(&backdrop));

    const QString serial = QStringLiteral("synthetic-visibility-test");
    std::unique_ptr<VideoForm> video(new VideoForm(false, false, true));
    video->setSerial(serial);
    video->setWindowTitle(QStringLiteral("QtScrcpy · 真实投屏窗口功能验证"));
    QCOMPARE(video->opacityPercent(), 100);
    QVERIFY(!video->autoHideEnabled());
    video->show();
    QVERIFY(QTest::qWaitForWindowExposed(video.get()));
    SyntheticYuv frame;
    frame.present(*video);
    QTest::qWait(150);
    frame.present(*video);
    positionVideo(*video);
    QTRY_VERIFY_WITH_TIMEOUT(video->findChild<ToolForm *>() != nullptr, 3000);
    ToolForm *toolbar = video->findChild<ToolForm *>();
    QTRY_VERIFY(toolbar->isVisible());
    toolbar->raise();
    QTest::qWait(150);

    auto renderer = video->findChild<QYUVOpenGLWidget *>();
    QVERIFY(renderer);
    QVERIFY(renderer->isValid());
    QCOMPARE(renderer->frameSize(), QSize(FrameWidth, FrameHeight));
    const QImage rendered = renderer->grabFramebuffer();
    QVERIFY(!rendered.isNull());
    const QColor top = rendered.pixelColor(rendered.width() / 2, rendered.height() / 10);
    const QColor bottom = rendered.pixelColor(rendered.width() / 2, rendered.height() * 9 / 10);
    QVERIFY2(colorDifference(top, bottom) > 120, "The production OpenGL widget must display the synthetic blue/red YUV frame.");
    const QPoint sample = renderer->mapToGlobal(QPoint(renderer->width() * 3 / 4, renderer->height() / 2));
    const QColor opaquePixel = screenPixel(sample);
    QVERIFY(opaquePixel.isValid());
    QVERIFY(saveScreenshot(*video, *toolbar, "100-percent"));

    auto opacityButton = toolbar->findChild<QPushButton *>("opacityBtn");
    auto autoHideButton = toolbar->findChild<QPushButton *>("autoHideBtn");
    auto slider = toolbar->findChild<QSlider *>("opacitySlider");
    auto menu = toolbar->findChild<QMenu *>("opacityMenu");
    auto valueLabel = toolbar->findChild<QLabel *>("opacityValueLabel");
    QVERIFY(opacityButton && autoHideButton && slider && menu && valueLabel);
    QVERIFY(autoHideButton->isCheckable());
    QCOMPARE(slider->minimum(), 10);
    QCOMPARE(slider->maximum(), 100);
    QSignalSpy opacityChanges(video.get(), &VideoForm::opacityPercentChanged);
    QCursor::setPos(opacityButton->mapToGlobal(opacityButton->rect().center()));
    QTest::mouseClick(opacityButton, Qt::LeftButton);
    QTRY_VERIFY(menu->isVisible());
    QCOMPARE(slider->value(), 100);
    slider->setFocus();
    QTest::keyClick(slider, Qt::Key_Home);
    QCOMPARE(video->opacityPercent(), 10);
    QTest::keyClick(slider, Qt::Key_PageUp);
    QCOMPARE(video->opacityPercent(), 20);
    QVERIFY(menu->isVisible());

    // Exercise the real slider handle, rather than only setValue().
    QStyleOptionSlider option;
    option.initFrom(slider);
    option.orientation = Qt::Horizontal;
    option.minimum = slider->minimum();
    option.maximum = slider->maximum();
    option.sliderPosition = slider->value();
    option.sliderValue = slider->value();
    const QRect handle = slider->style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, slider);
    QTest::mousePress(slider, Qt::LeftButton, Qt::NoModifier, handle.center());
    QTest::mouseMove(slider, QPoint(slider->width() * 2 / 3, handle.center().y()), 30);
    QTest::mouseRelease(slider, Qt::LeftButton, Qt::NoModifier, QPoint(slider->width() * 2 / 3, handle.center().y()));
    QVERIFY2(slider->value() > 30, "Dragging the actual production slider must change opacity.");
    QCOMPARE(video->opacityPercent(), slider->value());
    QVERIFY(menu->isVisible());
    QVERIFY(opacityChanges.count() >= 3);

    video->setOpacityPercent(40);
    QCOMPARE(slider->value(), 40);
    QVERIFY(valueLabel->text().contains("40%"));
    QVERIFY(approximately(video->windowOpacity(), 0.4));
    QVERIFY(approximately(toolbar->windowOpacity(), 0.4));
    QVERIFY(approximately(menu->windowOpacity(), 0.4));
    QTest::qWait(150);
    const QColor transparentPixel = screenPixel(sample);
    QVERIFY2(colorDifference(opaquePixel, transparentPixel) > 35,
             "Screen pixels must change when the actual OpenGL top-level window becomes transparent.");
    QVERIFY(saveScreenshot(*video, *toolbar, "40-percent-menu", true));
    menu->hide();

    video->setOpacityPercent(100);
    QTest::qWait(150);
    QVERIFY(approximately(video->windowOpacity(), 1.0));
    QVERIFY(approximately(toolbar->windowOpacity(), 1.0));
    const QColor restoredPixel = screenPixel(sample);
    QVERIFY2(colorDifference(opaquePixel, restoredPixel) < 20, "The rendered video must return after restoring opacity.");
    video->setOpacityPercent(40);
    QCursor::setPos(autoHideButton->mapToGlobal(autoHideButton->rect().center()));
    QTest::mouseClick(autoHideButton, Qt::LeftButton);
    QVERIFY(video->autoHideEnabled());
    QVERIFY(autoHideButton->isChecked());
    QCursor::setPos(renderer->mapToGlobal(renderer->rect().center()));
    QTest::qWait(150);
    QVERIFY(approximately(video->windowOpacity(), 0.4));
    const QPoint outside = QGuiApplication::primaryScreen()->availableGeometry().bottomRight() - QPoint(20, 20);
    QCursor::setPos(outside);
    QTRY_VERIFY_WITH_TIMEOUT(video->windowOpacity() < 0.01, 2000);
    QVERIFY(toolbar->windowOpacity() < 0.01);
    QVERIFY(video->isVisible());
    QVERIFY(toolbar->isVisible());
    QCursor::setPos(renderer->mapToGlobal(renderer->rect().center()));
    QTRY_VERIFY_WITH_TIMEOUT(approximately(video->windowOpacity(), 0.4), 2000);
    QVERIFY(approximately(toolbar->windowOpacity(), 0.4));

    // An owned menu is an interaction region even after the pointer leaves.
    QCursor::setPos(opacityButton->mapToGlobal(opacityButton->rect().center()));
    QTest::mouseClick(opacityButton, Qt::LeftButton);
    QTRY_VERIFY(menu->isVisible());
    QCursor::setPos(outside);
    QTest::qWait(650);
    QVERIFY(approximately(video->windowOpacity(), 0.4));
    QVERIFY(menu->isVisible());
    menu->hide();
    video->setAutoHideEnabled(false);
    QVERIFY(!autoHideButton->isChecked());
    video->setAutoHideEnabled(true);
    QVERIFY(autoHideButton->isChecked());
    QCursor::setPos(renderer->mapToGlobal(renderer->rect().center()));
    QTest::qWait(400); // Production save debounce is 250ms.
    QCOMPARE(Config::getInstance().getWindowVisibility(serial).opacityPercent, 40);
    QVERIFY(Config::getInstance().getWindowVisibility(serial).autoHideEnabled);
    video.reset();
    QCoreApplication::processEvents();

    VideoForm restored(false, false, true);
    restored.setSerial(serial);
    QCOMPARE(restored.opacityPercent(), 40);
    QVERIFY(restored.autoHideEnabled());
    restored.show();
    QVERIFY(QTest::qWaitForWindowExposed(&restored));
    frame.present(restored);
    positionVideo(restored);
    QTRY_VERIFY_WITH_TIMEOUT(restored.findChild<ToolForm *>() != nullptr, 3000);
    ToolForm *restoredToolbar = restored.findChild<ToolForm *>();
    QCOMPARE(restoredToolbar->findChild<QSlider *>("opacitySlider")->value(), 40);
    QVERIFY(restoredToolbar->findChild<QPushButton *>("autoHideBtn")->isChecked());
    restored.setAutoHideEnabled(false);
    QVERIFY(approximately(restored.windowOpacity(), 0.4));
    QVERIFY(approximately(restoredToolbar->windowOpacity(), 0.4));

    const QDir configDirectory(QString::fromLocal8Bit(qgetenv("QTSCRCPY_CONFIG_PATH")));
    QVERIFY(QFileInfo(configDirectory.filePath("userdata.ini")).isFile());
    QSettings saved(configDirectory.filePath("userdata.ini"), QSettings::IniFormat);
    QCOMPARE(saved.value(serial + "/WindowOpacityPercent").toInt(), 40);
}

void VisibilityUiTest::skinnedOpenGLComposition()
{
    CompositionBackdrop backdrop;
    backdrop.setGeometry(QGuiApplication::primaryScreen()->availableGeometry().adjusted(12, 12, -12, -12));
    backdrop.show();
    QVERIFY(QTest::qWaitForWindowExposed(&backdrop));

    VideoForm video(false, true, true);
    video.setSerial(QStringLiteral("synthetic-skinned-visibility-test"));
    video.setWindowTitle(QStringLiteral("QtScrcpy · 默认皮肤透明度验证"));
    video.show();
    QVERIFY(QTest::qWaitForWindowExposed(&video));
    SyntheticYuv frame;
    frame.present(video);
    QTest::qWait(150);
    frame.present(video);
    positionVideo(video);
    QTRY_VERIFY_WITH_TIMEOUT(video.findChild<ToolForm *>() != nullptr, 3000);
    auto toolbar = video.findChild<ToolForm *>();
    auto renderer = video.findChild<QYUVOpenGLWidget *>();
    QVERIFY(toolbar && renderer && renderer->isValid());
#ifndef Q_OS_MACOS
    QVERIFY(video.testAttribute(Qt::WA_TranslucentBackground));
#endif
    toolbar->raise();
    QTest::qWait(150);
    QVERIFY(!renderer->grabFramebuffer().isNull());
    const QPoint sample = renderer->mapToGlobal(QPoint(renderer->width() * 3 / 4, renderer->height() / 2));
    const QColor opaquePixel = screenPixel(sample);
    QVERIFY(saveScreenshot(video, *toolbar, "skin-100-percent"));
    video.setOpacityPercent(40);
    QTest::qWait(150);
    QVERIFY(approximately(video.windowOpacity(), 0.4));
    QVERIFY(approximately(toolbar->windowOpacity(), 0.4));
    QVERIFY2(colorDifference(opaquePixel, screenPixel(sample)) > 35,
             "The default translucent phone skin must also compose the OpenGL video at 40% opacity.");
    QVERIFY(saveScreenshot(video, *toolbar, "skin-40-percent"));
    video.setOpacityPercent(100);
    QTest::qWait(150);
    QVERIFY2(colorDifference(opaquePixel, screenPixel(sample)) < 20,
             "The default skinned OpenGL video must recover when opacity returns to 100%.");
}

int main(int argc, char *argv[])
{
    // Set both paths before any production singleton is constructed. The test
    // never reads or writes the real user's QtScrcpy configuration.
    QTemporaryDir isolatedConfig(QDir::tempPath() + "/QtScrcpy-visibility-test-XXXXXX");
    if (!isolatedConfig.isValid())
        return 2;
    const QByteArray configPath = isolatedConfig.path().toLocal8Bit();
    qputenv("QTSCRCPY_CONFIG_PATH", configPath);
    qputenv("QTSCRCPY_DEFAULT_CONFIG_PATH", configPath);
    QCoreApplication::setOrganizationName("QtScrcpyVisibilityTests");
    QCoreApplication::setApplicationName("QtScrcpyVisibilityTests");
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setVersion(2, 0);
    format.setProfile(QSurfaceFormat::NoProfile);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QFile stylesheet(":/qss/psblack.css");
    if (stylesheet.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#1e1e1e"));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor("#252525"));
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor("#252525"));
    palette.setColor(QPalette::ButtonText, Qt::white);
    app.setPalette(palette);
    const QPoint originalCursor = QCursor::pos();
    VisibilityUiTest tests;
    const int result = QTest::qExec(&tests, argc, argv);
    QCursor::setPos(originalCursor);
    return result;
}

#include "visibility_ui_test.moc"
