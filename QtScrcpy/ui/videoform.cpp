// #include <QDesktopWidget>
#include <QCoreApplication>
#include <QApplication>
#include <QCursor>
#include <QClipboard>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QFocusEvent>
#include <QHideEvent>
#include <QSizeGrip>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QShortcut>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QWindow>
#include <QRegion>
#include <QSettings>
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
#include <QAction>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>

#ifdef Q_OS_MACOS
#include "metalvideowindow.h"
#endif

namespace {
// QWidget::isAncestorOf stops at the native window boundaries used by chrome.
bool belongsToProjection(const QObject *object, const QObject *projection)
{
    for (auto *owner = object; owner; owner = owner->parent())
        if (owner == projection) return true;
    return false;
}
class ProjectionCornerGrip final : public QWidget {
public:
    explicit ProjectionCornerGrip(VideoForm *target):QWidget(target),m_target(target) {
        setObjectName("ProjectionTopLeftGrip");setFixedSize(14,14);
        setCursor(Qt::SizeFDiagCursor);setToolTip(QStringLiteral("拖动左上角等比例缩放"));
    }
protected:
    void mousePressEvent(QMouseEvent *e) override {
        if(e->button()!=Qt::LeftButton)return;
        m_start=m_target->geometry();m_press=e->globalPosition().toPoint();m_resizing=true;
        m_target->setChromeDragging(true);e->accept();
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if(!m_resizing||!(e->buttons()&Qt::LeftButton))return;
        const QPoint delta=e->globalPosition().toPoint()-m_press;
        const QSize originalPhone=m_target->phoneSizeForWindow(m_start.size());
        const qreal ratio=qreal(originalPhone.width())/originalPhone.height();
        int w=qRound(qAbs(delta.x())>=qAbs(delta.y())*ratio?originalPhone.width()-delta.x():(originalPhone.height()-delta.y())*ratio);
        int maxHeight=m_start.bottom()-40;
        if(auto *screen=QGuiApplication::screenAt(m_start.center()))maxHeight=m_start.bottom()-screen->availableGeometry().top()-(m_start.height()-originalPhone.height());
        w=qBound(qMin(160,qRound(maxHeight*ratio)),w,qMax(1,qRound(maxHeight*ratio)));
        const QSize size=m_target->windowSizeForPhone(QSize(w,qMax(1,qRound(w/ratio))));
        m_target->setGeometry(QRect(m_start.bottomRight()-QPoint(size.width()-1,size.height()-1),size));e->accept();
    }
    void mouseReleaseEvent(QMouseEvent *e) override {m_resizing=false;m_target->setChromeDragging(false);e->accept();}
private:
    VideoForm *m_target;QRect m_start;QPoint m_press;bool m_resizing=false;
};
class ProjectionTitle final : public QWidget {
public:
    explicit ProjectionTitle(VideoForm *target):QWidget(target),m_target(target){
        setObjectName(QStringLiteral("ProjectionTitle"));setAttribute(Qt::WA_ShowWithoutActivating);
        setStyleSheet(QStringLiteral("#ProjectionTitle{background:#f4f3ee;border:1px solid #d8ddd1;border-radius:7px;} QLabel{color:#52604f;background:transparent;}"));
    }
protected:
    void mousePressEvent(QMouseEvent *e) override {
        if(e->button()==Qt::LeftButton){m_pressGlobal=e->globalPosition().toPoint();m_offset=m_pressGlobal-m_target->frameGeometry().topLeft();m_dragging=true;m_started=false;m_target->setChromeDragging(true);e->accept();}
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if(!m_dragging||!(e->buttons()&Qt::LeftButton)||m_target->isFullScreen())return;
        const QPoint global=e->globalPosition().toPoint();
        if(!m_started){
            if((global-m_pressGlobal).manhattanLength()<QApplication::startDragDistance())return;
            m_target->restoreExpandedForDrag(global);m_started=true;
        }
        m_target->move(global-m_offset);e->accept();
    }
    void mouseReleaseEvent(QMouseEvent*) override {m_dragging=false;m_started=false;m_target->setChromeDragging(false);}
    void mouseDoubleClickEvent(QMouseEvent *e) override {
        if(e->button()!=Qt::LeftButton)return;
        m_dragging=false;m_started=false;m_target->setChromeDragging(false);m_target->toggleExpanded();e->accept();
    }
private:VideoForm *m_target;QPoint m_offset,m_pressGlobal;bool m_dragging=false,m_started=false;
};
}

VideoForm::VideoForm(bool framelessWindow, bool skin, bool showToolbar, int decodeMode, QWidget *parent) : QWidget(parent), ui(new Ui::videoForm), m_skin(false), m_decodeMode(decodeMode)
{
    Q_UNUSED(skin)
    Q_UNUSED(framelessWindow)
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::NoDropShadowWindowHint,true);
    setAttribute(Qt::WA_TranslucentBackground);
    ui->setupUi(this);
    setStyleSheet(QStringLiteral("#videoForm { background: transparent; border: none; border-image: none; }"));
    m_flexResizeTimer.setSingleShot(true);
    m_flexResizeTimer.setInterval(300);
    connect(&m_flexResizeTimer, &QTimer::timeout, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (device && device->isFlexDisplay() && !m_pendingDisplaySize.isEmpty()) {
            device->resizeDisplay(m_pendingDisplaySize);
        }
    });
    initUI();
    setAttribute(Qt::WA_InputMethodEnabled);
    setFocusPolicy(Qt::StrongFocus);
    if (videoWidget()) videoWidget()->setFocusProxy(this);
    layout()->setContentsMargins(0,0,0,0);
    layout()->removeWidget(ui->keepRatioWidget);
    createProjectionChrome();
    installShortcut();
    updateShowSize(size());
    bool vertical = size().height() > size().width();
    this->show_toolbar = showToolbar;
    m_holdToMove.setSingleShot(true);m_holdToMove.setInterval(400);
    connect(&m_holdToMove,&QTimer::timeout,this,[this](){
        if(!m_pendingPhonePress||m_cursorGrabbed||isFullScreen())return;
        m_pendingPhonePress=false;restoreExpandedForDrag(m_holdGlobal.toPoint());m_windowDragActive=true;
        m_dragPosition=m_holdGlobal.toPoint()-frameGeometry().topLeft();
        setCursor(Qt::ClosedHandCursor);
    });
    qApp->installEventFilter(this);
    m_toolIdleTime.start();
    m_lastToolCursor = QCursor::pos();
    m_aspectResizeTimer.setSingleShot(true);m_aspectResizeTimer.setInterval(0);
    connect(&m_aspectResizeTimer,&QTimer::timeout,this,[this](){
        if(isFullScreen()||m_fullscreenTransition||m_flexDisplay||m_pendingAspectSize.isEmpty())return;
        m_adjustingAspect=true;resize(m_pendingAspectSize);m_adjustingAspect=false;
    });
    m_toolVisibilityTimer.setInterval(100);
    connect(&m_toolVisibilityTimer, &QTimer::timeout, this, [this]() {
        if (m_sessionEnding || !isVisible() || isMinimized() || m_projectionMinimizing || m_cursorGrabbed) { hideProjectionControls(); return; }
        if (!videoWidget()) return;
        placeProjectionChrome();
        if(m_toolForm&&m_toolForm->isVisible())placeToolForm();
        const QPoint cursor=QCursor::pos();
        const bool moved=cursor!=m_lastToolCursor;m_lastToolCursor=cursor;
        const QRect viewport(videoWidget()->mapToGlobal(QPoint()),videoWidget()->size());
        const bool overTitle=m_titleChrome&&m_titleChrome->isVisible()&&QRect(m_titleChrome->mapToGlobal(QPoint()),m_titleChrome->size()).contains(cursor);
        const bool overTools=m_toolForm&&m_toolForm->isVisible()&&QRect(m_toolForm->mapToGlobal(QPoint()),m_toolForm->size()).contains(cursor);
        auto *popup=QApplication::activePopupWidget();
        const bool menu=popup&&belongsToProjection(popup,this);
        if(isFullScreen()) {
            const QPoint local=mapFromGlobal(cursor);
            const bool edge=rect().contains(local)&&(local.x()<12||local.x()>=width()-12||local.y()<12||local.y()>=height()-12);
            if(menu||overTools||(moved&&edge&&belongsToProjection(QApplication::activeWindow(),this)))showProjectionControls();
            else if(m_toolIdleTime.elapsed()>=2000)hideProjectionControls();
            return;
        }
        const bool interacting=m_chromeDragging||m_windowDragActive||menu||
            ((viewport.contains(cursor)||overTitle||overTools)&&QApplication::mouseButtons()!=Qt::NoButton);
        if(interacting||(moved&&belongsToProjection(QApplication::activeWindow(),this)&&(viewport.adjusted(-5,-34,5,0).contains(cursor)||overTitle||overTools))) {
            if(moved&&!interacting&&!overTools&&viewport.width()<viewport.height()) {
                if(cursor.x()<=viewport.left()+8)m_toolsOnRight=false;
                else if(cursor.x()>=viewport.right()-8)m_toolsOnRight=true;
            }
            showProjectionControls();
        }
        setProperty("chromeIdleMs",m_toolIdleTime.isValid()?m_toolIdleTime.elapsed():-1);
        setProperty("chromeInteraction",interacting);
        setProperty("chromeMoved",moved);setProperty("chromeCursor",cursor);
        if(m_toolIdleTime.isValid()&&m_toolIdleTime.elapsed()>=5000&&!interacting)hideProjectionControls();
    });
    m_toolVisibilityTimer.start();
    if (m_skin) {
        updateStyleSheet(vertical);
    }


    // 监听显示器增删：副屏被拔掉时，位于该屏幕上的窗口会变成"幽灵窗口"
    // （坐标仍在已断开的屏幕区域）。这里在屏幕配置变化后做一次越界检查，
    // 必要时把窗口移回主屏。延迟执行是为了等 Qt/系统完成屏幕布局更新。
    //
    // Windows 上 screenRemoved 的触发时机与屏幕布局更新并不完全同步，
    // 因此做多次延迟重试，确保在布局稳定后仍能完成回收。
    connect(qApp, &QGuiApplication::screenAdded, this, [this]() {
        QTimer::singleShot(300, this, [this]() { ensureOnScreen(); });
        QTimer::singleShot(1000, this, [this]() { ensureOnScreen(); });
    });
    connect(qApp, &QGuiApplication::screenRemoved, this, [this]() {
        QTimer::singleShot(300, this, [this]() { ensureOnScreen(); });
        QTimer::singleShot(1000, this, [this]() { ensureOnScreen(); });
        QTimer::singleShot(2000, this, [this]() { ensureOnScreen(); });
    });
}

VideoForm::~VideoForm()
{
    m_toolVisibilityTimer.stop();
    delete ui;
}

bool VideoForm::isMetalMode() const
{
#ifdef Q_OS_MACOS
    return !m_metalWidget.isNull();
#else
    return false;
#endif
}

QWidget* VideoForm::videoWidget() const
{
#ifdef Q_OS_MACOS
    if (isMetalMode()) {
        return m_metalWidget.data();
    }
#endif
    return m_videoWidget.data();
}

void VideoForm::initUI()
{
    if (m_skin) {
        QPixmap phone;
        if (phone.load(":/res/phone.png")) {
            m_widthHeightRatio = 1.0f * phone.width() / phone.height();
        }

#ifndef Q_OS_MACOS
        // mac下去掉标题栏影响showfullscreen
        // 去掉标题栏
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        // 根据图片构造异形窗口
        setAttribute(Qt::WA_TranslucentBackground);
#endif
    }

#ifdef Q_OS_MACOS
    // Apple Silicon: 使用 VideoToolbox + Metal 渲染
    if (m_decodeMode == 1) {
        m_metalWidget = new MetalVideoWidget();
        ui->keepRatioWidget->setWidget(m_metalWidget);

        // FPS label 作为 Metal widget 的子控件
        m_fpsLabel = new QLabel(m_metalWidget);
    } else
#endif
    {
        // OpenGL 路径（原有逻辑）
        m_videoWidget = new QYUVOpenGLWidget();
        m_videoWidget->hide();
        ui->keepRatioWidget->setWidget(m_videoWidget);

        // FPS label 作为 OpenGL widget 的子控件
        m_fpsLabel = new QLabel(m_videoWidget);
    }

    ui->keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);

    QFont ft;
    ft.setPointSize(15);
    ft.setWeight(QFont::Light);
    ft.setBold(true);
    m_fpsLabel->setFont(ft);
    m_fpsLabel->move(5, 15);
    m_fpsLabel->setMinimumWidth(100);
    m_fpsLabel->setStyleSheet(R"(QLabel {color: #00FF00;})");

    setMouseTracking(true);
    if (m_videoWidget) {
        m_videoWidget->setMouseTracking(true);
    }
    ui->keepRatioWidget->setMouseTracking(true);
}

QRect VideoForm::getGrabCursorRect()
{
    QRect rc;
    QWidget *vw = videoWidget();
#if defined(Q_OS_WIN32)
    rc = QRect(ui->keepRatioWidget->mapToGlobal(vw->pos()), vw->size());
    // high dpi support
    rc.setTopLeft(rc.topLeft() * vw->devicePixelRatioF());
    rc.setBottomRight(rc.bottomRight() * vw->devicePixelRatioF());

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#elif defined(Q_OS_MACOS)
    rc = vw->geometry();
    rc.setTopLeft(ui->keepRatioWidget->mapToGlobal(rc.topLeft()));
    rc.setBottomRight(ui->keepRatioWidget->mapToGlobal(rc.bottomRight()));

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#elif defined(Q_OS_LINUX)
    rc = QRect(ui->keepRatioWidget->mapToGlobal(vw->pos()), vw->size());
    // high dpi support -- taken from the WIN32 section and untested
    rc.setTopLeft(rc.topLeft() * vw->devicePixelRatioF());
    rc.setBottomRight(rc.bottomRight() * vw->devicePixelRatioF());

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
    resize(windowSizeForPhone(ui->keepRatioWidget->goodSize()));
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
    if (isMetalMode()) {
        // Metal 路径不通过此方法渲染，使用 onFrameMetal
        return;
    }

    if (!m_videoWidget) {
        return;
    }

    if (m_videoWidget->isHidden()) {
        if (m_loadingWidget) {
            m_loadingWidget->close();
        }
        m_videoWidget->show();
    }

    if (!m_flexDisplay) {
        updateShowSize(QSize(width, height));
    } else {
        m_frameSize = QSize(width, height);
    }
    m_videoWidget->setFrameSize(QSize(width, height));
    m_videoWidget->updateTextures(dataY, dataU, dataV, linesizeY, linesizeU, linesizeV);
}

void VideoForm::setSerial(const QString &serial)
{
    m_serial = serial;
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    m_flexDisplay = device && device->isFlexDisplay();
    if (m_flexDisplay) {
        ui->keepRatioWidget->setWidthHeightRatio(-1.0f);
    }
}

void VideoForm::createProjectionChrome()
{
    auto *title=new ProjectionTitle(this);m_titleChrome=title;
    auto *bar=new QHBoxLayout(title);bar->setContentsMargins(10,4,10,4);bar->setSpacing(7);
    auto dot=[&](const QString &name,const QString &color){auto *b=new QPushButton; b->setFixedSize(13,13);b->setAccessibleName(name);b->setToolTip(name);
        b->setStyleSheet(QStringLiteral("QPushButton{background:%1;border:none;border-radius:6px;padding:0;} QPushButton:hover{border:1px solid #65725c;}").arg(color));bar->addWidget(b);return b;};
    auto *close=dot(QStringLiteral("结束此投屏"),QStringLiteral("#db8d80"));
    auto *minimize=dot(QStringLiteral("最小化投屏"),QStringLiteral("#cfb16b"));
    auto *full=dot(QStringLiteral("切换全屏"),QStringLiteral("#8eaa85"));
    connect(close,&QPushButton::clicked,this,&QWidget::close);connect(minimize,&QPushButton::clicked,this,&VideoForm::minimizeProjection);
    connect(full,&QPushButton::clicked,this,&VideoForm::switchFullScreen);
    auto *caption=new QLabel(QStringLiteral("投屏"));m_chromeTitle=caption;caption->setAlignment(Qt::AlignCenter);caption->setAttribute(Qt::WA_TransparentForMouseEvents);bar->addWidget(caption,1);
    connect(this,&QWidget::windowTitleChanged,caption,&QLabel::setText);
    title->setFixedHeight(34);
    m_topLeftGrip=new ProjectionCornerGrip(this);
    m_resizeGrip=new QSizeGrip(this);m_resizeGrip->setFixedSize(22,22);m_resizeGrip->setToolTip(QStringLiteral("拖动调整投屏窗口大小"));
    auto *reveal=new QShortcut(QKeySequence(QStringLiteral("Ctrl+Alt+T")),this);
    connect(reveal,&QShortcut::activated,this,[this](){if(m_cursorGrabbed)grabCursor(false);showProjectionControls();});
}

QMargins VideoForm::projectionInsets() const
{
    if(isFullScreen())return QMargins();
    const bool landscape=m_frameSize.width()>m_frameSize.height();
    return QMargins(46,landscape?80:34,46,0);
}

QSize VideoForm::windowSizeForPhone(const QSize &size) const
{
    const auto margins=projectionInsets();
    return size+QSize(margins.left()+margins.right(),margins.top()+margins.bottom());
}

QSize VideoForm::phoneSizeForWindow(const QSize &size) const
{
    const auto margins=projectionInsets();
    return QSize(qMax(1,size.width()-margins.left()-margins.right()),qMax(1,size.height()-margins.top()-margins.bottom()));
}

QRect VideoForm::phoneRect() const
{
    if(auto *widget=videoWidget())return QRect(widget->mapTo(const_cast<VideoForm*>(this),QPoint()),widget->size());
    return rect().marginsRemoved(projectionInsets());
}

void VideoForm::updateProjectionMask()
{
    if(!videoWidget())return;
    if(isFullScreen()){if(!mask().isEmpty()){clearMask();update();}return;}
    QRegion visible(phoneRect());
    if(m_titleChrome&&m_titleChrome->isVisible())visible+=m_titleChrome->geometry();
    if(m_toolForm&&m_toolForm->isVisible())visible+=m_toolForm->geometry();
    if(mask()!=visible){setMask(visible);update();}
}

void VideoForm::placeProjectionChrome()
{
    if(!m_titleChrome||!videoWidget())return;
    ui->keepRatioWidget->setGeometry(rect().marginsRemoved(projectionInsets()));
    const QRect phone=phoneRect();
    m_titleChrome->setGeometry(phone.x(),phone.y()-34,phone.width(),34);
    if(m_topLeftGrip){m_topLeftGrip->move(phone.topLeft());m_topLeftGrip->raise();}
    if(m_resizeGrip){m_resizeGrip->move(phone.bottomRight()-QPoint(21,21));m_resizeGrip->raise();}
    if(isFullScreen()){m_titleChrome->hide();if(m_resizeGrip)m_resizeGrip->hide();if(m_topLeftGrip)m_topLeftGrip->hide();}
    updateProjectionMask();
}

void VideoForm::placeToolForm()
{
    if(!m_toolForm||!videoWidget())return;
    const QRect phone=phoneRect();
    m_toolForm->setFullscreenMode(isFullScreen());
    if(isFullScreen()) {
        // Prefer a black side bar, then a top/bottom letterbox; otherwise overlay
        // the top edge without reducing or shifting the phone image.
        const int side=qMax(phone.left(),width()-phone.right()-1);
        const bool vertical=side>=62;
        m_toolForm->setHorizontal(!vertical);
        int x=(width()-m_toolForm->width())/2,y=12;
        if(vertical){x=phone.left()>=62?(phone.left()-m_toolForm->width())/2:phone.right()+1+(width()-phone.right()-1-m_toolForm->width())/2;y=(height()-m_toolForm->height())/2;}
        else if(phone.top()>=62)y=(phone.top()-m_toolForm->height())/2;
        else if(height()-phone.bottom()-1>=62)y=phone.bottom()+1+(height()-phone.bottom()-1-m_toolForm->height())/2;
        m_toolForm->move(qBound(8,x,qMax(8,width()-m_toolForm->width()-8)),qBound(8,y,qMax(8,height()-m_toolForm->height()-8)));
        m_toolForm->raise();updateProjectionMask();return;
    }
    const bool landscape=phone.width()>phone.height();m_toolForm->setHorizontal(landscape);
    const int x=landscape?phone.center().x()-m_toolForm->width()/2:(m_toolsOnRight?phone.right()+1:phone.left()-m_toolForm->width());
    const int y=landscape?(isFullScreen()?phone.top():phone.top()-80):phone.top();
    m_toolForm->move(qBound(0,x,qMax(0,width()-m_toolForm->width())),qBound(0,y,qMax(0,height()-m_toolForm->height())));
    m_toolForm->raise();
    updateProjectionMask();
}

void VideoForm::showToolForm(bool show)
{
    if(show&&(m_sessionEnding||!isVisible()||isMinimized()||m_projectionMinimizing||m_cursorGrabbed))show=false;
    if(!m_toolForm){m_toolForm=new ToolForm(this,MagneticWidget::AP_OUTSIDE_LEFT);m_toolForm->setSerial(m_serial);m_toolForm->setAudioState(m_audioActive,m_audioStarting);connect(m_toolForm,&ToolForm::audioToggleRequested,this,&VideoForm::audioToggleRequested);}
    if(show){placeToolForm();}
    m_toolForm->setVisible(show);
    updateProjectionMask();
}

void VideoForm::setAudioState(bool active,bool starting)
{
    m_audioActive=active;m_audioStarting=starting;
    if(m_toolForm)m_toolForm->setAudioState(active,starting);
}

void VideoForm::showProjectionControls()
{
    if(m_sessionEnding||m_cursorGrabbed||!isVisible()||isMinimized()||m_projectionMinimizing)return;
    m_toolIdleTime.restart();
    placeProjectionChrome();showToolForm(show_toolbar);
    if(m_titleChrome)m_titleChrome->setVisible(!isFullScreen());
    if(m_resizeGrip)m_resizeGrip->setVisible(!isFullScreen());
    if(m_topLeftGrip)m_topLeftGrip->setVisible(!isFullScreen());
    if(m_titleChrome)m_titleChrome->raise();
    if(m_toolForm)m_toolForm->raise();
    updateProjectionMask();
}

void VideoForm::hideProjectionControls()
{
    if(m_toolForm)m_toolForm->hide();
    if(m_titleChrome)m_titleChrome->hide();
    if(m_resizeGrip)m_resizeGrip->hide();
    // Keep the transparent corner hit target usable even when toolbar chrome is idle-hidden.
    if(m_topLeftGrip)m_topLeftGrip->setVisible(isVisible()&&!isMinimized()&&!m_projectionMinimizing&&!isFullScreen()&&!m_cursorGrabbed);
    updateProjectionMask();
}

void VideoForm::minimizeProjection()
{
    m_projectionMinimizing=true;
    m_chromeDragging=false;
    m_holdToMove.stop();m_pendingPhonePress=false;m_windowDragActive=false;
    hideProjectionControls();
    showMinimized();
}

void VideoForm::setChromeDragging(bool dragging)
{
    m_chromeDragging=dragging;
    showProjectionControls();
}

bool VideoForm::eventFilter(QObject *watched,QEvent *event)
{
    if(event->type()==QEvent::ApplicationDeactivate ||
       (event->type()==QEvent::WindowDeactivate&&belongsToProjection(watched,this))) {
        // macOS can briefly change activation while native tool windows exchange focus.
        QTimer::singleShot(0,this,[this](){
            if(!m_chromeDragging&&(QGuiApplication::applicationState()!=Qt::ApplicationActive ||
               !belongsToProjection(QApplication::activeWindow(),this)))
                hideProjectionControls();
        });
    }
    // Native QWindow mouse delivery precedes QWidget delivery. It has a separate
    // ownership tree: treating it as an outside widget hides then immediately
    // shows chrome again on every phone click.
    auto *widget=qobject_cast<QWidget*>(watched);
    if(!widget)return QWidget::eventFilter(watched,event);
    const bool own=belongsToProjection(widget,this);
    if(event->type()==QEvent::MouseButtonPress) {
        if(own) {
            // User interaction raises the whole projection group; hover never steals focus.
            if(!QApplication::activePopupWidget()&&!belongsToProjection(QApplication::activeWindow(),this)) {
                raise();activateWindow();
                if(m_titleChrome&&m_titleChrome->isVisible())m_titleChrome->raise();
                if(m_toolForm&&m_toolForm->isVisible())m_toolForm->raise();
            }
            if(!isFullScreen()||belongsToProjection(widget,m_toolForm))showProjectionControls();
        }
        else if(!QApplication::activePopupWidget()&&!m_chromeDragging)hideProjectionControls();
    } else if(own&&!isFullScreen()&&(event->type()==QEvent::KeyPress||event->type()==QEvent::Wheel))showProjectionControls();
    return QWidget::eventFilter(watched,event);
}

void VideoForm::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    // Chrome is child content: native window movement carries it atomically.
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

bool VideoForm::isRectOnAnyScreen(const QRect &rect) const
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (!screen) {
            continue;
        }
        // 用 intersects 而非 contains：窗口只要有一部分可见就认为在屏内，
        // 允许用户把窗口放在屏幕边缘（部分露出）的情况。
        if (screen->availableGeometry().intersects(rect)) {
            return true;
        }
    }
    return false;
}

void VideoForm::ensureOnScreen()
{
    if(isFullScreen()||m_fullscreenTransition)return;
    auto *target=QGuiApplication::screenAt(frameGeometry().center());
    if(!target)target=QGuiApplication::screenAt(QCursor::pos());
    if(!target)target=QGuiApplication::primaryScreen();
    if(!target)return;
    const QRect safe=target->availableGeometry().adjusted(8,8,-8,-8);
    const QSize limit=phoneSizeForWindow(safe.size());
    QSize phone=phoneSizeForWindow(size());
    if(!m_frameSize.isEmpty())phone.setHeight(qMax(1,qRound(phone.width()/m_widthHeightRatio)));
    if(phone.width()>limit.width()||phone.height()>limit.height())phone.scale(limit,Qt::KeepAspectRatio);
    m_aspectResizeTimer.stop();m_adjustingAspect=true;resize(windowSizeForPhone(phone));m_adjustingAspect=false;
    if(!safe.contains(geometry()))move(safe.center()-rect().center());
    placeProjectionChrome();placeToolForm();
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

    shortcut = new QShortcut(QKeySequence("Ctrl+Alt+n"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (device) {
            device->expandSettingsPanel();
        }
    });

    shortcut = new QShortcut(QKeySequence("Ctrl+r"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (device) {
            device->rotateDevice();
        }
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

    // clipboardPaste
    shortcut = new QShortcut(QKeySequence("Ctrl+v"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->setDeviceClipboard();
    });

    // setDeviceClipboard
    shortcut = new QShortcut(QKeySequence("Ctrl+Shift+v"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (!device) {
            return;
        }
        emit device->clipboardPaste();
    });
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

        if(isFullScreen()) {
            // Rotation updates aspect-fit content inside the same fullscreen host.
            placeProjectionChrome();placeToolForm();update();return;
        }

        if (isMaximized()) {
            showNormal();
        }

        if (m_skin) {
            QMargins m = getMargins(vertical);
            showSize.setWidth(showSize.width() + m.left() + m.right());
            showSize.setHeight(showSize.height() + m.top() + m.bottom());
        }

        showSize=windowSizeForPhone(showSize);
        if (showSize != size()) {
            resize(showSize);
            if (m_skin) {
                updateStyleSheet(vertical);
            }
            moveCenter();
        }
    }
}

void VideoForm::onVideoSessionChanged(const QSize &size, bool clientResized)
{
    if (m_flexDisplay) {
        m_frameSize = size;
        m_preventAutoResize = clientResized;
        ui->keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);
        return;
    }
    // clientResized is only meaningful for flex display. Normal display
    // rotations must retain the longstanding auto-resize behavior.
    m_preventAutoResize = false;
    updateShowSize(size);
}

void VideoForm::toggleExpanded()
{
    m_holdToMove.stop();m_pendingPhonePress=false;m_windowDragActive=false;unsetCursor();
    switchFullScreen();
}

void VideoForm::restoreExpandedForDrag(const QPoint &global)
{
    if(!isFullScreen())return;
    const qreal fraction=qBound(0.0,qreal(global.x()-x())/qMax(1,width()),1.0);
    toggleExpanded();
    move(global-QPoint(qRound(width()*fraction),-20));
}

void VideoForm::switchFullScreen()
{
    if(m_fullscreenTransition){m_pendingFullscreenToggle=!m_pendingFullscreenToggle;return;}
    m_fullscreenTransition=true;
    const int transitionId=++m_fullscreenTransitionId;
    m_aspectResizeTimer.stop();
    setMinimumSize(0,0);
    if (isFullScreen()) {
        // 横屏全屏铺满全屏，恢复时，恢复保持宽高比
        if (m_widthHeightRatio > 1.0f) {
            ui->keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);
        }

        layout()->setContentsMargins(0,0,0,0);
        showNormal();
        // back to normal size.
        resize(m_normalSize);
        // fullscreen window will move (0,0). qt bug?
        move(m_fullScreenBeforePos);

#ifdef Q_OS_MACOS
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
        // Fullscreen must preserve the source aspect ratio, including landscape.
        ui->keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);

        // record current size before fullscreen, it will be used to rollback size after exit fullscreen.
        m_normalSize = size();

        m_fullScreenBeforePos = pos();
        // 这种临时增加标题栏再全屏的方案会导致收不到mousemove事件，导致setmousetrack失效
        // mac fullscreen must show title bar
#ifdef Q_OS_MACOS
        //setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
#endif
        showToolForm(false);
        if (m_skin) {
            layout()->setContentsMargins(0, 0, 0, 0);
        }
        layout()->setContentsMargins(0,0,0,0);
        hideProjectionControls();
        showFullScreen();
        placeProjectionChrome();placeToolForm();

        // 全屏状态禁止电脑休眠、息屏
#ifdef Q_OS_WIN32
        ::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#endif
    }
    m_aspectResizeTimer.stop();
    const QRect restore(m_fullScreenBeforePos,m_normalSize);
    QTimer::singleShot(1500,this,[this,transitionId,restore](){
        if(transitionId!=m_fullscreenTransitionId)return;
        if(!isFullScreen()){setGeometry(restore);placeProjectionChrome();showProjectionControls();}
        else {placeProjectionChrome();placeToolForm();updateProjectionMask();}
        m_aspectResizeTimer.stop();m_fullscreenTransition=false;
        if(m_pendingFullscreenToggle){m_pendingFullscreenToggle=false;switchFullScreen();}
    });
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
    if (!m_fpsLabel) {
        return;
    }
    m_fpsLabel->setText(QString("FPS:%1").arg(fps));
}

void VideoForm::grabCursor(bool grab)
{
    m_cursorGrabbed = grab;
    setAttribute(Qt::WA_InputMethodEnabled, desktopImeEnabled());
    QGuiApplication::inputMethod()->update(Qt::ImEnabled);
    if (grab) hideProjectionControls();
    QRect rc = getGrabCursorRect();
    MouseTap::getInstance()->enableMouseEventTap(rc, grab);
}

void VideoForm::onFrame(int width, int height, uint8_t *dataY, uint8_t *dataU, uint8_t *dataV, int linesizeY, int linesizeU, int linesizeV)
{
    updateRender(width, height, dataY, dataU, dataV, linesizeY, linesizeU, linesizeV);
}

void VideoForm::onFrameMetal(void *cvPixelBuffer, int width, int height)
{
#ifdef Q_OS_MACOS
    if (!m_metalWidget || !cvPixelBuffer) {
        return;
    }

    if (m_metalFirstFrame) {
        m_metalFirstFrame = false;
        if (m_loadingWidget) {
            m_loadingWidget->close();
        }
        ui->keepRatioWidget->updateGeometry();
    }

    updateShowSize(QSize(width, height));
    m_metalWidget->renderFrame((CVPixelBufferRef)cvPixelBuffer, width, height);
#else
    Q_UNUSED(cvPixelBuffer);
    Q_UNUSED(width);
    Q_UNUSED(height);
#endif
}

void VideoForm::staysOnTop(bool top)
{
    bool needShow = false;
    if (isVisible()) {
        needShow = true;
    }
    setWindowFlag(Qt::WindowStaysOnTopHint, top);

    if (needShow) {
        show();
    }
}

void VideoForm::mousePressEvent(QMouseEvent *event)
{
    if (desktopImeEnabled()) {
        setFocus(Qt::MouseFocusReason);
        m_imeAnchor = event->position().toPoint();
        QGuiApplication::inputMethod()->update(Qt::ImCursorRectangle | Qt::ImEnabled);
    }
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

    QWidget *vw = videoWidget();
    if(event->button()==Qt::LeftButton&&!m_cursorGrabbed&&!isFullScreen()
       &&vw&&QRect(vw->mapTo(this,QPoint()),vw->size()).contains(event->pos())&&device&&!device->isCurrentCustomKeymap()){
        m_pendingPhonePress=true;m_holdLocal=localPos;m_holdGlobal=globalPos;m_holdToMove.start();event->accept();return;
    }
    if (vw && QRect(vw->mapTo(this,QPoint()),vw->size()).contains(event->pos())) {
        if (!device) {
            return;
        }
        QPointF mappedPos = vw->mapFrom(this, localPos.toPoint());
        QMouseEvent newEvent(event->type(), mappedPos, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_frameSize, vw->size());

    } else {
        if (event->button() == Qt::LeftButton) {
            m_dragPosition = globalPos.toPoint() - frameGeometry().topLeft();
            event->accept();
        }
    }
}

void VideoForm::mouseReleaseEvent(QMouseEvent *event)
{
    m_holdToMove.stop();
    if(m_windowDragActive){m_windowDragActive=false;m_dragPosition=QPoint();unsetCursor();event->accept();return;}
    if(m_pendingPhonePress){m_pendingPhonePress=false;forwardHeldPhonePress();}
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
        QWidget *vw = videoWidget();
        if (!vw) {
            return;
        }

        // local check
        QPointF local = vw->mapFrom(this, localPos.toPoint());
        if (local.x() < 0) {
            local.setX(0);
        }
        if (local.x() > vw->width()) {
            local.setX(vw->width());
        }
        if (local.y() < 0) {
            local.setY(0);
        }
        if (local.y() > vw->height()) {
            local.setY(vw->height());
        }
        QMouseEvent newEvent(event->type(), local, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_frameSize, vw->size());
    } else {
        m_dragPosition = QPoint(0, 0);
    }
}

void VideoForm::forwardHeldPhonePress()
{
    auto device=qsc::IDeviceManage::getInstance().getDevice(m_serial);auto *vw=videoWidget();
    if(!device||!vw)return;
    QMouseEvent press(QEvent::MouseButtonPress,vw->mapFrom(this,m_holdLocal.toPoint()),m_holdGlobal,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    emit device->mouseEvent(&press,m_frameSize,vw->size());
}

void VideoForm::mouseMoveEvent(QMouseEvent *event)
{
    if(m_windowDragActive){if(event->buttons()&Qt::LeftButton)move(event->globalPosition().toPoint()-m_dragPosition);event->accept();return;}
    if(m_pendingPhonePress){
        if((event->globalPosition()-m_holdGlobal).manhattanLength()<QApplication::startDragDistance()){event->accept();return;}
        m_holdToMove.stop();m_pendingPhonePress=false;forwardHeldPhonePress();
    }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QPointF localPos = event->localPos();
        QPointF globalPos = event->globalPos();
#else
        QPointF localPos = event->position();
        QPointF globalPos = event->globalPosition();
#endif
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    QWidget *vw = videoWidget();
    if (vw && QRect(vw->mapTo(this,QPoint()),vw->size()).contains(event->pos())) {
        if (!device) {
            return;
        }
        QPointF mappedPos = vw->mapFrom(this, localPos.toPoint());
        QMouseEvent newEvent(event->type(), mappedPos, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_frameSize, vw->size());
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
    QWidget *vw = videoWidget();
    if (event->button() == Qt::LeftButton && vw && !QRect(vw->mapTo(this,QPoint()),vw->size()).contains(event->pos())) {
        toggleExpanded();event->accept();return;
    }

    if (event->button() == Qt::RightButton && device && !device->isCurrentCustomKeymap()) {
        emit device->postBackOrScreenOn(event->type() == QEvent::MouseButtonPress);
    }

    if (vw && QRect(vw->mapTo(this,QPoint()),vw->size()).contains(event->pos())) {
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
        QPointF mappedPos = vw->mapFrom(this, localPos.toPoint());
        QMouseEvent newEvent(event->type(), mappedPos, globalPos, event->button(), event->buttons(), event->modifiers());
        emit device->mouseEvent(&newEvent, m_frameSize, vw->size());
    }
}

void VideoForm::wheelEvent(QWheelEvent *event)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    QWidget *vw = videoWidget();
    if (!vw) {
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (phoneRect().contains(event->position().toPoint())) {
        if (!device) {
            return;
        }
        QPointF pos = vw->mapFrom(this, event->position().toPoint());
        QWheelEvent wheelEvent(
            pos, event->globalPosition(), event->pixelDelta(), event->angleDelta(), event->buttons(), event->modifiers(), event->phase(), event->inverted());
#else
    if (QRect(vw->mapTo(this,QPoint()),vw->size()).contains(event->pos())) {
        if (!device) {
            return;
        }
        QPointF pos = vw->mapFrom(this, event->pos());

        QWheelEvent wheelEvent(
            pos, event->globalPosF(), event->pixelDelta(), event->angleDelta(), event->delta(), event->orientation(),
            event->buttons(), event->modifiers(), event->phase(), event->source(), event->inverted());
#endif
        emit device->wheelEvent(&wheelEvent, m_frameSize, vw->size());
    }
}

bool VideoForm::desktopImeEnabled() const
{
    const auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    return !m_cursorGrabbed && (!device || !device->isCurrentCustomKeymap());
}

QVariant VideoForm::inputMethodQuery(Qt::InputMethodQuery query) const
{
    if (query == Qt::ImEnabled) return desktopImeEnabled();
    if (query == Qt::ImCursorRectangle) return QRect(m_imeAnchor, QSize(2, 24));
    if (query == Qt::ImHints) return int(Qt::ImhNoPredictiveText);
    if (query == Qt::ImSurroundingText || query == Qt::ImCurrentSelection) return QString();
    if (query == Qt::ImCursorPosition || query == Qt::ImAnchorPosition) return 0;
    return QWidget::inputMethodQuery(query);
}

void VideoForm::inputMethodEvent(QInputMethodEvent *event)
{
    if (!desktopImeEnabled()) { event->ignore(); return; }
    const QString text = event->commitString();
    if (!text.isEmpty()) {
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (device) {
            // The existing Unicode clipboard protocol supports Chinese unlike key injection.
            // Never log committed text: it may contain private messages.
            QApplication::clipboard()->setText(text);
            device->setDeviceClipboard(true);
        }
    }
    m_imePreedit = event->preeditString();
    // macOS owns the candidate UI; a second label overlaps its popup.
    event->accept();
}

void VideoForm::focusOutEvent(QFocusEvent *event)
{
    QGuiApplication::inputMethod()->reset();
    m_imePreedit.clear();
    QWidget::focusOutEvent(event);
}

bool VideoForm::event(QEvent *event)
{
    if(event->type()==QEvent::WindowStateChange) {
        m_projectionMinimizing=isMinimized();
        if(m_projectionMinimizing)hideProjectionControls();
        else if(isVisible())showProjectionControls();
    }
    // QWidget otherwise consumes Tab for focus traversal before keyPressEvent.
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        auto *key = static_cast<QKeyEvent *>(event);
        auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
        if (device && device->isCurrentCustomKeymap() && key->key() == Qt::Key_Tab) {
            if (event->type() == QEvent::KeyPress) keyPressEvent(key);
            else keyReleaseEvent(key);
            event->accept();
            return true;
        }
    }
    return QWidget::event(event);
}

void VideoForm::keyPressEvent(QKeyEvent *event)
{
    if (desktopImeEnabled() && !m_imePreedit.isEmpty()) { event->accept(); return; }
    m_forwardedKeys.insert(event->key());
    if(Qt::Key_Escape==event->key()&&!event->isAutoRepeat()&&isFullScreen()){switchFullScreen();event->accept();return;}
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if(!device)return;

    QWidget *vw = videoWidget();
    QSize widgetSize = vw ? vw->size() : m_frameSize;
    emit device->keyEvent(event, m_frameSize, widgetSize);
}

void VideoForm::keyReleaseEvent(QKeyEvent *event)
{
    if (desktopImeEnabled() && !m_forwardedKeys.contains(event->key())) { event->accept(); return; }
    if (!event->isAutoRepeat()) m_forwardedKeys.remove(event->key());
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    QWidget *vw = videoWidget();
    QSize widgetSize = vw ? vw->size() : m_frameSize;
    emit device->keyEvent(event, m_frameSize, widgetSize);
}

void VideoForm::paintEvent(QPaintEvent *paint)
{
    Q_UNUSED(paint)
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(),Qt::transparent);
    painter.fillRect(isFullScreen()?rect():phoneRect(),Qt::black);
}

void VideoForm::hideEvent(QHideEvent *event)
{
    hideProjectionControls();
    QWidget::hideEvent(event);
}

void VideoForm::showEvent(QShowEvent *event)
{
    if(!isMinimized())m_projectionMinimizing=false;
    Q_UNUSED(event)
    // 窗口每次显示时先做一次越界检查，处理"上次退出时窗口位于副屏、
    // 本次启动副屏已不存在"导致窗口出现在屏幕外的情况。
    //
    // Dialog::onDeviceConnected 会先 setGeometry(savedRect) 再 show()，
    // 因此这里能捕获到从配置中恢复出来的屏外坐标。
    ensureOnScreen();
    // 窗口显示后 Qt 仍可能根据尺寸/边框策略再做一次位置调整，
    // 故稍后复查一次，确保最终位置落在屏内。
    QTimer::singleShot(300, this, [this]() { ensureOnScreen(); });
    if (m_titleChrome) showProjectionControls();
    if (!isFullScreen() && this->show_toolbar) {
        QTimer::singleShot(500, this, [this](){
            showToolForm(this->show_toolbar);
        });
    }
}

void VideoForm::resizeEvent(QResizeEvent *event)
{
    placeProjectionChrome();
    placeToolForm();
    Q_UNUSED(event)
    if (m_flexDisplay) {
        m_pendingDisplaySize = ui->keepRatioWidget->size();
        if (!m_pendingDisplaySize.isEmpty()) {
            m_flexResizeTimer.start();
        }
        return;
    }

    if(!m_adjustingAspect&&!m_fullscreenTransition&&!isFullScreen()&&m_widthHeightRatio>0.0f){
        const QSize requested=phoneSizeForWindow(event->size());
        if(qAbs(requested.width()-requested.height()*m_widthHeightRatio)>1.5){
            const QSize previous=phoneSizeForWindow(event->oldSize());
            const bool changedWidth=previous.isEmpty()||qAbs(requested.width()-previous.width())>=qAbs(requested.height()-previous.height())*m_widthHeightRatio;
            m_pendingAspectSize=changedWidth?QSize(requested.width(),qRound(requested.width()/m_widthHeightRatio)):QSize(qRound(requested.height()*m_widthHeightRatio),requested.height());
            m_pendingAspectSize=windowSizeForPhone(m_pendingAspectSize);
            m_aspectResizeTimer.start();
        }
    }
}

void VideoForm::saveWindowGeometry()
{
    if(m_serial.isEmpty())return;
    const QRect bounds=isFullScreen()?QRect(m_fullScreenBeforePos,m_normalSize):geometry();
    Config::getInstance().setRect(m_serial,bounds);
    QSettings().setValue("projection/geometryVersion/"+m_serial,2);
}

void VideoForm::finishSession()
{
    saveWindowGeometry();
    m_sessionEnding=true;
    hideProjectionControls();
    close();
}

void VideoForm::closeEvent(QCloseEvent *event)
{
    event->accept();
    hideProjectionControls();
    if(m_sessionEnding)return;
    m_sessionEnding=true;
    saveWindowGeometry();
    emit sessionCloseRequested(m_serial);
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
