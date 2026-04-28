#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QInputMethodEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QProcess>
#include <QRegularExpression>
#include <QScreen>
#include <QShortcut>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QWindow>
#include <QtWidgets/QHBoxLayout>

#if defined(Q_OS_WIN32)
#include <Windows.h>
#endif

#include "config.h"
#include "iconhelper.h"
#include "qyuvopenglwidget.h"
#include "toolform.h"
#include "mousetap/mousetap.h"
#include "ui_videoform.h"
#include "videoform.h"

VideoForm::VideoForm(bool framelessWindow, bool skin, bool showToolbar, QWidget *parent) : QWidget(parent), ui(new Ui::videoForm), m_skin(skin)
{
    ui->setupUi(this);
    initUI();
    installShortcut();
    updateShowSize(size());
    bool vertical = size().height() > size().width();
    this->show_toolbar = showToolbar;
    if (m_skin) {
        updateStyleSheet(vertical);
    }
    if (framelessWindow) {
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    }
}

VideoForm::~VideoForm()
{
    delete ui;
}

void VideoForm::initUI()
{
    if (m_skin) {
        QPixmap phone;
        if (phone.load(":/res/phone.png")) {
            m_widthHeightRatio = 1.0f * phone.width() / phone.height();
        }

#ifndef Q_OS_OSX
        // mac下去掉标题栏影响showfullscreen
        // 去掉标题栏
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        // 根据图片构造异形窗口
        setAttribute(Qt::WA_TranslucentBackground);
#endif
    }

    m_videoWidget = new QYUVOpenGLWidget();
    m_videoWidget->hide();
    ui->keepRatioWidget->setWidget(m_videoWidget);
    ui->keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);

    m_fpsLabel = new QLabel(m_videoWidget);
    QFont ft;
    ft.setPointSize(15);
    ft.setWeight(QFont::Light);
    ft.setBold(true);
    m_fpsLabel->setFont(ft);
    m_fpsLabel->move(5, 15);
    m_fpsLabel->setMinimumWidth(100);
    m_fpsLabel->setStyleSheet(R"(QLabel {color: #00FF00;})");

    setMouseTracking(true);
    m_videoWidget->setMouseTracking(true);
    ui->keepRatioWidget->setMouseTracking(true);
}

QRect VideoForm::getGrabCursorRect()
{
    QRect rc;
#if defined(Q_OS_WIN32)
    rc = QRect(ui->keepRatioWidget->mapToGlobal(m_videoWidget->pos()), m_videoWidget->size());
    // high dpi support
    rc.setTopLeft(rc.topLeft() * m_videoWidget->devicePixelRatioF());
    rc.setBottomRight(rc.bottomRight() * m_videoWidget->devicePixelRatioF());

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#elif defined(Q_OS_OSX)
    rc = m_videoWidget->geometry();
    rc.setTopLeft(ui->keepRatioWidget->mapToGlobal(rc.topLeft()));
    rc.setBottomRight(ui->keepRatioWidget->mapToGlobal(rc.bottomRight()));

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#elif defined(Q_OS_LINUX)
    rc = QRect(ui->keepRatioWidget->mapToGlobal(m_videoWidget->pos()), m_videoWidget->size());
    // high dpi support -- taken from the WIN32 section and untested
    rc.setTopLeft(rc.topLeft() * m_videoWidget->devicePixelRatioF());
    rc.setBottomRight(rc.bottomRight() * m_videoWidget->devicePixelRatioF());

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#endif
    return rc;
}

const QSize &VideoForm::frameSize()
{
    return m_frameSize;
}

void VideoForm::resizeSquare()
{
    QRect screenRect = getScreenRect();
    if (screenRect.isEmpty()) {
        qWarning() << "getScreenRect is empty";
        return;
    }
    resize(screenRect.height(), screenRect.height());
}

void VideoForm::removeBlackRect()
{
    resize(ui->keepRatioWidget->goodSize());
}

void VideoForm::showFPS(bool show)
{
    if (!m_fpsLabel) {
        return;
    }
    m_fpsLabel->setVisible(show);
}

void VideoForm::updateRender(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV, int linesizeY, int linesizeU, int linesizeV)
{
    if (m_videoWidget->isHidden()) {
        if (m_loadingWidget) {
            m_loadingWidget->close();
        }
        m_videoWidget->show();
    }

    updateShowSize(QSize(width, height));
    m_videoWidget->setFrameSize(QSize(width, height));
    m_videoWidget->updateTextures(dataY, dataU, dataV, linesizeY, linesizeU, linesizeV);
}

void VideoForm::setSerial(const QString &serial)
{
    m_serial = serial;
}

void VideoForm::showToolForm(bool show)
{
    if (!m_toolForm) {
        m_toolForm = new ToolForm(this, ToolForm::AP_OUTSIDE_RIGHT);
        m_toolForm->setSerial(m_serial);
    }
    m_toolForm->move(pos().x() + geometry().width(), pos().y() + 30);
    m_toolForm->setVisible(show);
}

void VideoForm::moveCenter()
{
    QRect screenRect = getScreenRect();
    if (screenRect.isEmpty()) {
        qWarning() << "getScreenRect is empty";
        return;
    }
    // 窗口居中
    move(screenRect.center() - QRect(0, 0, size().width(), size().height()).center());
}

void VideoForm::installShortcut()
{
    QShortcut *shortcut = nullptr;

    // switchFullScreen
    shortcut = new QShortcut(QKeySequence("Ctrl+f"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        switchFullScreen();
    });

    // resizeSquare
    shortcut = new QShortcut(QKeySequence("Ctrl+g"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { resizeSquare(); });

    // removeBlackRect
    shortcut = new QShortcut(QKeySequence("Ctrl+w"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { removeBlackRect(); });

    // postGoHome
    shortcut = new QShortcut(QKeySequence("Ctrl+h"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        device->postGoHome();
    });

    // postGoBack
    shortcut = new QShortcut(QKeySequence("Ctrl+b"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        device->postGoBack();
    });

    // postAppSwitch
    shortcut = new QShortcut(QKeySequence("Ctrl+s"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postAppSwitch();
    });

    // postGoMenu
    shortcut = new QShortcut(QKeySequence("Ctrl+m"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        device->postGoMenu();
    });

    // postVolumeUp
    shortcut = new QShortcut(QKeySequence("Ctrl+up"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postVolumeUp();
    });

    // postVolumeDown
    shortcut = new QShortcut(QKeySequence("Ctrl+down"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postVolumeDown();
    });

    // postPower
    shortcut = new QShortcut(QKeySequence("Ctrl+p"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postPower();
    });

    shortcut = new QShortcut(QKeySequence("Ctrl+o"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->setDisplayPower(false);
    });

    // expandNotificationPanel
    shortcut = new QShortcut(QKeySequence("Ctrl+n"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->expandNotificationPanel();
    });

    // collapsePanel
    shortcut = new QShortcut(QKeySequence("Ctrl+Shift+n"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->collapsePanel();
    });

    // copy
    shortcut = new QShortcut(QKeySequence("Ctrl+c"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postCopy();
    });

    // cut
    shortcut = new QShortcut(QKeySequence("Ctrl+x"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->postCut();
    });

    // clipboardPaste (PC clipboard changed -> auto sync to Android)
    // Remove manual Ctrl+V shortcut, replace with automatic clipboard monitoring
    // by connecting to QApplication::clipboard()->dataChanged()
    // The old Ctrl+Shift+v for clipboardPaste is also removed - now automatic

    // --- Auto clipboard monitor: PC -> Android (no shortcut needed) ---
    QClipboard *clipboard = QApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        // Anti-loop: if the current clipboard text matches what we last pushed from Android,
        // this dataChanged was triggered by Receiver setting PC clipboard (Android->PC echo),
        // so skip pushing it back to Android to avoid infinite loop.
        QString currentText = QApplication::clipboard()->text();
        if (!m_lastPushedToAndroid.isEmpty() && currentText == m_lastPushedToAndroid) {
            m_lastPushedToAndroid.clear();
            return;
        }
        m_lastPushedToAndroid = currentText;
        emit device->setDeviceClipboard(false); // false = don't pause device video
    });

    // --- Auto clipboard monitor: Android -> PC ---
    // Already handled by Receiver::recvDeviceMsg which sets QApplication::clipboard()
    // when it receives DMT_GET_CLIPBOARD from Android server
}


QRect VideoForm::getScreenRect()
{
    QRect screenRect;
    QScreen *screen = QGuiApplication::primaryScreen();
    QWidget *win = window();
    if (win) {
        QWindow *winHandle = win->windowHandle();
        if (winHandle) {
            screen = winHandle->screen();
        }
    }

    if (screen) {
        screenRect = screen->availableGeometry();
    }
    return screenRect;
}

void VideoForm::updateStyleSheet(bool vertical)
{
    if (vertical) {
        setStyleSheet(R"(
                 #videoForm {
                     border-image: url(:/image/videoform/phone-v.png) 150px 65px 85px 65px;
                     border-width: 150px 65px 85px 65px;
                 }
                 )");
    } else {
        setStyleSheet(R"(
                 #videoForm {
                     border-image: url(:/image/videoform/phone-h.png) 65px 85px 65px 150px;
                     border-width: 65px 85px 65px 150px;
                 }
                 )");
    }
    layout()->setContentsMargins(getMargins(vertical));
}

QMargins VideoForm::getMargins(bool vertical)
{
    QMargins margins;
    if (vertical) {
        margins = QMargins(10, 68, 12, 62);
    } else {
        margins = QMargins(68, 12, 62, 10);
    }
    return margins;
}

void VideoForm::updateShowSize(const QSize &newSize)
{
    if (m_frameSize != newSize) {
        m_frameSize = newSize;

        m_widthHeightRatio = 1.0f * newSize.width() / newSize.height();
        ui->keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);

        bool vertical = m_widthHeightRatio < 1.0f ? true : false;
        QSize showSize = newSize;
        QRect screenRect = getScreenRect();
        if (screenRect.isEmpty()) {
            qWarning() << "getScreenRect is empty";
            return;
        }
        if (vertical) {
            showSize.setHeight(qMin(newSize.height(), screenRect.height() - 200));
            showSize.setWidth(showSize.height() * m_widthHeightRatio);
        } else {
            showSize.setWidth(qMin(newSize.width(), screenRect.width() / 2));
            showSize.setHeight(showSize.width() / m_widthHeightRatio);
        }

        if (isFullScreen() && qsc::IDeviceManage::getInstance().getDevice(m_serial)) {
            switchFullScreen();
        }

        if (isMaximized()) {
            showNormal();
        }

        if (m_skin) {
            QMargins m = getMargins(vertical);
            showSize.setWidth(showSize.width() + m.left() + m.right());
            showSize.setHeight(showSize.height() + m.top() + m.bottom());
        }

        if (showSize != size()) {
            resize(showSize);
            if (m_skin) {
                updateStyleSheet(vertical);
            }
            moveCenter();
        }
    }
}

void VideoForm::switchFullScreen()
{
    if (isFullScreen()) {
        // 横屏全屏铺满全屏，恢复时，恢复保持宽高比
        if (m_widthHeightRatio > 1.0f) {
            ui->keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);
        }

        showNormal();
        // back to normal size.
        resize(m_normalSize);
        // fullscreen window will move (0,0). qt bug?
        move(m_fullScreenBeforePos);

#ifdef Q_OS_OSX
        //setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        //show();
#endif
        if (m_skin) {
            updateStyleSheet(m_frameSize.height() > m_frameSize.width());
        }
        showToolForm(this->show_toolbar);
#ifdef Q_OS_WIN32
        ::SetThreadExecutionState(ES_CONTINUOUS);
#endif
    } else {
        // 横屏全屏铺满全屏，不保持宽高比
        if (m_widthHeightRatio > 1.0f) {
            ui->keepRatioWidget->setWidthHeightRatio(-1.0f);
        }

        // record current size before fullscreen, it will be used to rollback size after exit fullscreen.
        m_normalSize = size();

        m_fullScreenBeforePos = pos();
        // 这种临时增加标题栏再全屏的方案会导致收不到mousemove事件，导致setmousetrack失效
        // mac fullscreen must show title bar
#ifdef Q_OS_OSX
        //setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
#endif
        showToolForm(false);
        if (m_skin) {
            layout()->setContentsMargins(0, 0, 0, 0);
        }
        showFullScreen();

        // 全屏状态禁止电脑休眠、息屏
#ifdef Q_OS_WIN32
        ::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#endif
    }
}

bool VideoForm::isHost()
{
    if (!m_toolForm) {
        return false;
    }
    return m_toolForm->isHost();
}

void VideoForm::updateFPS(quint32 fps)
{
    //qDebug() << "FPS:" << fps;
    if (!m_fpsLabel) {
        return;
    }
    m_fpsLabel->setText(QString("FPS:%1").arg(fps));
}

void VideoForm::grabCursor(bool grab)
{
    QRect rc = getGrabCursorRect();
    MouseTap::getInstance()->enableMouseEventTap(rc, grab);
}

void VideoForm::onFrame(int width, int height, uint8_t *dataY, uint8_t *dataU, uint8_t *dataV, int linesizeY, int linesizeU, int linesizeV)
{
    updateRender(width, height, dataY, dataU, dataV, linesizeY, linesizeU, linesizeV);
}

void VideoForm::staysOnTop(bool top)
{
    bool needShow = false;
    if (isVisible()) {
        needShow = true;
    }
    setWindowFlag(Qt::WindowStaysOnTopHint, top);
    if (m_toolForm) {
        m_toolForm->setWindowFlag(Qt::WindowStaysOnTopHint, top);
    }
    if (needShow) {
        show();
    }
}

void VideoForm::mousePressEvent(QMouseEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (event->button() == Qt::MiddleButton) {
        if (device && !device->isCurrentCustomKeymap()) {
            device->postGoHome();
            return;
        }
    }

    if (event->button() == Qt::RightButton) {
        if (device && !device->isCurrentCustomKeymap()) {
            device->postGoBack();
            return;
        }
    }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QPointF localPos = event->localPos();
        QPointF globalPos = event->globalPos();
#else
        QPointF localPos = event->position();
        QPointF globalPos = event->globalPosition();
#endif

    if (m_videoWidget->geometry().contains(event->pos())) {
        if (!device) {
            return;
        }
        QPointF mappedPos = m_videoWidget->mapFrom(this, localPos.toPoint());
        QMouseEvent newEvent(event->type(), mappedPos, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_videoWidget->frameSize(), m_videoWidget->size());

        // debug keymap pos
        if (event->button() == Qt::LeftButton) {
            qreal x = localPos.x() / m_videoWidget->size().width();
            qreal y = localPos.y() / m_videoWidget->size().height();
            QString posTip = QString(R"("pos": {"x": %1, "y": %2})").arg(x).arg(y);
            qInfo() << posTip.toStdString().c_str();

            // Record click position in Android coords for IME cursor positioning
            if (m_imeSwitched && m_videoWidget->frameSize().isValid()) {
                QPointF widgetPos = m_videoWidget->mapFrom(this, localPos.toPoint());
                m_lastClickAndroid.setX(qRound(widgetPos.x() * m_videoWidget->frameSize().width() / m_videoWidget->width()));
                m_lastClickAndroid.setY(qRound(widgetPos.y() * m_videoWidget->frameSize().height() / m_videoWidget->height()));
                // Set initial IME cursor position at click point
                m_imeCursorRect = QRectF(localPos.x() - 1, localPos.y() - 16, 2, 16);
                QInputMethod *im = qApp->inputMethod();
                if (im) im->update(Qt::ImCursorRectangle);
                // Async query focused input bounds via UIAutomator for precise positioning
                queryFocusedInputBounds();
            }
        }
    } else {
        if (event->button() == Qt::LeftButton) {
            m_dragPosition = globalPos.toPoint() - frameGeometry().topLeft();
            event->accept();
        }
    }
}

void VideoForm::mouseReleaseEvent(QMouseEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (m_dragPosition.isNull()) {
        if (!device) {
            return;
        }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QPointF localPos = event->localPos();
        QPointF globalPos = event->globalPos();
#else
        QPointF localPos = event->position();
        QPointF globalPos = event->globalPosition();
#endif
        // local check
        QPointF local = m_videoWidget->mapFrom(this, localPos.toPoint());
        if (local.x() < 0) {
            local.setX(0);
        }
        if (local.x() > m_videoWidget->width()) {
            local.setX(m_videoWidget->width());
        }
        if (local.y() < 0) {
            local.setY(0);
        }
        if (local.y() > m_videoWidget->height()) {
            local.setY(m_videoWidget->height());
        }
        QMouseEvent newEvent(event->type(), local, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_videoWidget->frameSize(), m_videoWidget->size());
    } else {
        m_dragPosition = QPoint(0, 0);
    }
}

void VideoForm::mouseMoveEvent(QMouseEvent *event)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QPointF localPos = event->localPos();
        QPointF globalPos = event->globalPos();
#else
        QPointF localPos = event->position();
        QPointF globalPos = event->globalPosition();
#endif
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (m_videoWidget->geometry().contains(event->pos())) {
        if (!device) {
            return;
        }
        QPointF mappedPos = m_videoWidget->mapFrom(this, localPos.toPoint());
        QMouseEvent newEvent(event->type(), mappedPos, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_videoWidget->frameSize(), m_videoWidget->size());
    } else if (!m_dragPosition.isNull()) {
        if (event->buttons() & Qt::LeftButton) {
            move(globalPos.toPoint() - m_dragPosition);
            event->accept();
        }
    }
}

void VideoForm::mouseDoubleClickEvent(QMouseEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (event->button() == Qt::LeftButton && !m_videoWidget->geometry().contains(event->pos())) {
        if (!isMaximized()) {
            removeBlackRect();
        }
    }

    if (event->button() == Qt::RightButton && device && !device->isCurrentCustomKeymap()) {
        emit device->postBackOrScreenOn(event->type() == QEvent::MouseButtonPress);
    }

    if (m_videoWidget->geometry().contains(event->pos())) {
        if (!device) {
            return;
        }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QPointF localPos = event->localPos();
        QPointF globalPos = event->globalPos();
#else
        QPointF localPos = event->position();
        QPointF globalPos = event->globalPosition();
#endif
        QPointF mappedPos = m_videoWidget->mapFrom(this, localPos.toPoint());
        QMouseEvent newEvent(event->type(), mappedPos, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_videoWidget->frameSize(), m_videoWidget->size());
    }
}

void VideoForm::wheelEvent(QWheelEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (m_videoWidget->geometry().contains(event->position().toPoint())) {
        if (!device) {
            return;
        }
        QPointF pos = m_videoWidget->mapFrom(this, event->position().toPoint());
        QWheelEvent wheelEvent(
            pos, event->globalPosition(), event->pixelDelta(), event->angleDelta(), event->buttons(), event->modifiers(), event->phase(), event->inverted());
#else
    if (m_videoWidget->geometry().contains(event->pos())) {
        if (!device) {
            return;
        }
        QPointF pos = m_videoWidget->mapFrom(this, event->pos());

        QWheelEvent wheelEvent(
            pos, event->globalPosF(), event->pixelDelta(), event->angleDelta(), event->delta(), event->orientation(),
            event->buttons(), event->modifiers(), event->phase(), event->source(), event->inverted());
#endif
        emit device->wheelEvent(&wheelEvent, m_videoWidget->frameSize(), m_videoWidget->size());
    }
}

void VideoForm::keyPressEvent(QKeyEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    if (Qt::Key_Escape == event->key() && !event->isAutoRepeat() && isFullScreen()) {
        switchFullScreen();
    }

    // If AdbKeyboard is active, inject printable characters via clipboard+paste
    // This makes PC keyboard input go to the phone reliably (no phone soft keyboard)
    if (m_imeSwitched && !event->text().isEmpty() && !event->isAutoRepeat()) {
        // Only inject for printable characters (letters, numbers, symbols, space)
        // Skip control keys (Ctrl, Alt, Meta combos) - let those go as keyEvent
        if (!(event->modifiers() & Qt::ControlModifier) && 
            !(event->modifiers() & Qt::MetaModifier)) {
            QString text = event->text();
            device->setClipboardAndPaste(text);
            return;
        }
    }

    emit device->keyEvent(event, m_videoWidget->frameSize(), m_videoWidget->size());
}

void VideoForm::keyReleaseEvent(QKeyEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    emit device->keyEvent(event, m_videoWidget->frameSize(), m_videoWidget->size());
}

void VideoForm::inputMethodEvent(QInputMethodEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        event->ignore();
        return;
    }

    // When Chinese IME (e.g. Sogou on PC) commits text, inject via clipboard+paste
    if (!event->commitString().isEmpty()) {
        QString text = event->commitString();
        device->setClipboardAndPaste(text);
        event->accept();
        return;
    }

    event->ignore();
}

void VideoForm::paintEvent(QPaintEvent *paint)
{
    Q_UNUSED(paint)
    QStyleOption opt;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    opt.init(this);
#else
    opt.initFrom(this);
#endif
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void VideoForm::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if (!isFullScreen() && this->show_toolbar) {
        QTimer::singleShot(500, this, [this](){
            showToolForm(this->show_toolbar);
        });
    }
    // Switch to AdbKeyboard after device is shown (delay to ensure serial is set)
    if (!m_imeSwitched && !m_serial.isEmpty()) {
        QTimer::singleShot(1000, this, [this](){
            switchToAdbKeyboard();
        });
    }
    // Enable Qt input method for Chinese input support
    setAttribute(Qt::WA_InputMethodEnabled, true);
}

void VideoForm::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    QSize goodSize = ui->keepRatioWidget->goodSize();
    if (goodSize.isEmpty()) {
        return;
    }
    QSize curSize = size();
    // 限制VideoForm尺寸不能小于keepRatioWidget good size
    if (m_widthHeightRatio > 1.0f) {
        // hor
        if (curSize.height() <= goodSize.height()) {
            setMinimumHeight(goodSize.height());
        } else {
            setMinimumHeight(0);
        }
    } else {
        // ver
        if (curSize.width() <= goodSize.width()) {
            setMinimumWidth(goodSize.width());
        } else {
            setMinimumWidth(0);
        }
    }

    // Reposition IME cursor when window resizes (fullscreen, maximize, etc.)
    if (m_imeSwitched && !m_lastInputBounds.isEmpty()) {
        QPointF topLeft = androidToFormPos(m_lastInputBounds.left(), m_lastInputBounds.top());
        QPointF bottomRight = androidToFormPos(m_lastInputBounds.right(), m_lastInputBounds.bottom());
        m_imeCursorRect = QRectF(topLeft.x(), topLeft.y(), qMax(bottomRight.x() - topLeft.x(), 2.0), qMax(bottomRight.y() - topLeft.y(), 16.0));
        QInputMethod *im = qApp->inputMethod();
        if (im) im->update(Qt::ImCursorRectangle);
    }
}

void VideoForm::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    Config::getInstance().setRect(device->getSerial(), geometry());
    // Restore original IME before disconnecting
    restoreOriginalIme();
    device->disconnectDevice();
}

// --- IME Management ---

void VideoForm::switchToAdbKeyboard()
{
    if (m_serial.isEmpty()) {
        return;
    }

    // Find adb executable path
    QString adbPath;
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    // Get adb path from the app's directory (adb is deployed alongside the app)
    adbPath = QCoreApplication::applicationDirPath() + "/adb";

    // Step 1: Save current default IME
    QStringList args;
    args << "-s" << m_serial << "shell" << "settings" << "get" << "secure" << "default_input_method";
    QProcess proc;
    proc.start(adbPath, args);
    proc.waitForFinished(3000);
    QString currentIme = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (!currentIme.isEmpty() && currentIme != "com.android.adbkeyboard/.AdbIME") {
        m_originalIme = currentIme;
        qInfo() << "Original IME saved:" << m_originalIme;
    }

    // Step 2: Install AdbKeyboard if not already installed
    // Check if AdbKeyboard is already installed
    args.clear();
    args << "-s" << m_serial << "shell" << "pm" << "list" << "packages" << "com.android.adbkeyboard";
    proc.start(adbPath, args);
    proc.waitForFinished(3000);
    QString packages = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

    if (!packages.contains("com.android.adbkeyboard")) {
        // Install AdbKeyboard.apk from the app directory
        QString apkPath = QCoreApplication::applicationDirPath() + "/AdbKeyboard.apk";
        if (QFileInfo::exists(apkPath)) {
            args.clear();
            args << "-s" << m_serial << "install" << "-r" << apkPath;
            proc.start(adbPath, args);
            proc.waitForFinished(15000);
            qInfo() << "AdbKeyboard installed:" << proc.readAllStandardOutput();
        } else {
            qWarning() << "AdbKeyboard.apk not found at:" << apkPath;
        }
    }

    // Step 3: Enable AdbKeyboard
    args.clear();
    args << "-s" << m_serial << "shell" << "ime" << "enable" << "com.android.adbkeyboard/.AdbIME";
    proc.start(adbPath, args);
    proc.waitForFinished(3000);

    // Step 4: Switch to AdbKeyboard
    args.clear();
    args << "-s" << m_serial << "shell" << "ime" << "set" << "com.android.adbkeyboard/.AdbIME";
    proc.start(adbPath, args);
    proc.waitForFinished(3000);

    // Step 5: Disable "show IME with hard keyboard" to prevent soft keyboard popup
    args.clear();
    args << "-s" << m_serial << "shell" << "settings" << "put" << "secure" << "show_ime_with_hard_keyboard" << "0";
    proc.start(adbPath, args);
    proc.waitForFinished(3000);

    // Step 6: Also disable show_ime_with_hard_keyboard in system (Android 14+)
    args.clear();
    args << "-s" << m_serial << "shell" << "settings" << "put" << "system" << "show_ime_with_hard_keyboard" << "0";
    proc.start(adbPath, args);
    proc.waitForFinished(3000);

    // Step 7: Verify IME switch was successful
    args.clear();
    args << "-s" << m_serial << "shell" << "settings" << "get" << "secure" << "default_input_method";
    proc.start(adbPath, args);
    proc.waitForFinished(3000);
    QString verifyIme = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

    if (verifyIme == "com.android.adbkeyboard/.AdbIME") {
        m_imeSwitched = true;
        qInfo() << "Switched to AdbKeyboard - soft keyboard will not appear on phone";
    } else {
        qWarning() << "Failed to switch to AdbKeyboard, current IME:" << verifyIme;
        // Try once more with a small delay
        QTimer::singleShot(500, this, [this]() {
            QString adbPath2 = QCoreApplication::applicationDirPath() + "/adb";
            QStringList args2;
            args2 << "-s" << m_serial << "shell" << "ime" << "set" << "com.android.adbkeyboard/.AdbIME";
            QProcess::execute(adbPath2, args2);
            m_imeSwitched = true;
        });
    }
}

void VideoForm::restoreOriginalIme()
{
    if (!m_imeSwitched || m_serial.isEmpty()) {
        return;
    }

    QString adbPath = QCoreApplication::applicationDirPath() + "/adb";

    // Try to restore to Sogou first (user's known IME), then fall back to saved IME
    QString targetIme = "com.sohu.inputmethod.sogou/.SogouIME";
    if (!m_originalIme.isEmpty()) {
        targetIme = m_originalIme;
    }

    QStringList args;
    args << "-s" << m_serial << "shell" << "ime" << "set" << targetIme;
    QProcess proc;
    proc.start(adbPath, args);
    proc.waitForFinished(3000);

    // Re-enable "show IME with hard keyboard" for normal use
    args.clear();
    args << "-s" << m_serial << "shell" << "settings" << "put" << "secure" << "show_ime_with_hard_keyboard" << "1";
    proc.start(adbPath, args);
    proc.waitForFinished(3000);

    m_imeSwitched = false;
    qInfo() << "Restored original IME:" << targetIme;
}

QString VideoForm::runAdbCommand(const QString &serial, const QStringList &args)
{
    QString adbPath = QCoreApplication::applicationDirPath() + "/adb";
    QProcess proc;
    proc.start(adbPath, QStringList() << "-s" << serial << args);
    proc.waitForFinished(5000);
    return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
}

// --- IME Cursor Positioning (no visible overlay, just tell PC IME where the cursor is) ---

QPointF VideoForm::androidToFormPos(int ax, int ay)
{
    if (!m_videoWidget || !m_frameSize.isValid()) {
        return QPointF();
    }
    // Android coord -> VideoWidget coord
    qreal wx = ax * m_videoWidget->width() / qreal(m_frameSize.width());
    qreal wy = ay * m_videoWidget->height() / qreal(m_frameSize.height());
    // VideoWidget coord -> VideoForm coord (handles black border offset + DPI)
    QPoint mapped = m_videoWidget->mapTo(this, QPoint(qRound(wx), qRound(wy)));
    return QPointF(mapped);
}

void VideoForm::queryFocusedInputBounds()
{
    if (m_serial.isEmpty()) return;

    QString adbPath = QCoreApplication::applicationDirPath() + "/adb";

    // Run uiautomator dump asynchronously
    QProcess* proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode) {
                if (exitCode != 0) {
                    proc->deleteLater();
                    return;
                }

                QString xml = QString::fromUtf8(proc->readAllStandardOutput());
                QRegularExpression re(
                    R"(class="android\.widget\.EditText"[^>]*bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"[^>]*focused="true")");
                QRegularExpressionMatch match = re.match(xml);

                if (!match.hasMatch()) {
                    QRegularExpression re2(
                        R"(class="android\.widget\.EditText"[^>]*focused="true"[^>]*bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]")");
                    match = re2.match(xml);
                }

                if (!match.hasMatch()) {
                    QRegularExpression re3(
                        R"(focused="true"[^>]*bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]")");
                    match = re3.match(xml);
                }

                if (match.hasMatch()) {
                    int left = match.captured(1).toInt();
                    int top = match.captured(2).toInt();
                    int right = match.captured(3).toInt();
                    int bottom = match.captured(4).toInt();
                    QRect bounds(left, top, right - left, bottom - top);
                    m_lastInputBounds = bounds;

                    // Convert Android input bounds to PC screen coordinates
                    QPointF topLeft = androidToFormPos(bounds.left(), bounds.top());
                    QPointF bottomRight = androidToFormPos(bounds.right(), bounds.bottom());
                    m_imeCursorRect = QRectF(topLeft.x(), topLeft.y(),
                                             qMax(bottomRight.x() - topLeft.x(), 2.0),
                                             qMax(bottomRight.y() - topLeft.y(), 16.0));

                    // Tell Qt IME to reposition its candidate window
                    QInputMethod *im = qApp->inputMethod();
                    if (im) im->update(Qt::ImCursorRectangle);

                    qInfo() << "IME cursor repositioned to UIAutomator bounds:" << bounds
                            << "-> PC pos:" << m_imeCursorRect;
                }

                proc->deleteLater();
            });

    proc->start(adbPath, QStringList()
                << "-s" << m_serial
                << "shell"
                << "uiautomator" << "dump" << "/dev/tty" << "2>/dev/null");
}

QVariant VideoForm::inputMethodQuery(Qt::InputMethodQuery query) const
{
    switch (query) {
    case Qt::ImCursorRectangle:
        // Tell the PC input method where to show its candidate window
        // This is the key: returns the phone input field's position on PC screen
        return m_imeCursorRect;
    case Qt::ImEnabled:
        // Enable IME input so inputMethodEvent gets called
        return m_imeSwitched;
    case Qt::ImHints:
        // No special hints (allow all IME features including Chinese input)
        return static_cast<int>(Qt::ImhNone);
    default:
        return QWidget::inputMethodQuery(query);
    }
}

void VideoForm::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void VideoForm::dragMoveEvent(QDragMoveEvent *event)
{
    Q_UNUSED(event)
}

void VideoForm::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
}

void VideoForm::dropEvent(QDropEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    const QMimeData *qm = event->mimeData();
    QList<QUrl> urls = qm->urls();

    for (const QUrl &url : urls) {
        QString file = url.toLocalFile();
        QFileInfo fileInfo(file);

        if (!fileInfo.exists()) {
            QMessageBox::warning(this, "QtScrcpy", tr("file does not exist"), QMessageBox::Ok);
            continue;
        }

        if (fileInfo.isFile() && fileInfo.suffix() == "apk") {
            emit device->installApkRequest(file);
            continue;
        }
        emit device->pushFileRequest(file, Config::getInstance().getPushFilePath() + fileInfo.fileName());
    }
}
