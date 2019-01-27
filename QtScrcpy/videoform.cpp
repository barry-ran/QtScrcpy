#include <QDesktopWidget>
#include <QMouseEvent>
#include <QTimer>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#ifdef Q_OS_WIN32
#include <Windows.h>
#endif
#include <QQuickWidget>
#include <QMimeData>
#include <QFileInfo>
#include <QMessageBox>

#include "videoform.h"
#include "ui_videoform.h"
#include "iconhelper.h"
#include "toolform.h"
#include "controlevent.h"
#include "recorder.h"

VideoForm::VideoForm(const QString& serial, quint16 maxSize, quint32 bitRate, const QString& fileName, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::videoForm),
    m_serial(serial),
    m_maxSize(maxSize),
    m_bitRate(bitRate)
{    
    ui->setupUi(this);    
    initUI();

    m_server = new Server();
    m_frames.init();
    m_decoder.setFrames(&m_frames);
    if (!fileName.trimmed().isEmpty()) {
        m_recorder = new Recorder(fileName.trimmed());
        m_decoder.setRecoder(m_recorder);
    }

    initSignals();

    // fix: macos cant recv finished signel, timer is ok
    QTimer::singleShot(0, this, [this](){
        bool sendFrameMeta = m_recorder ? true : false;
        m_startTimeCount.start();
        // max size support 480p 720p 1080p 设备原生分辨率
        // support wireless connect, example:
        //m_server->start("192.168.0.174:5555", 27183, m_maxSize, m_bitRate, "");
        // only one devices, serial can be null
        // mark: crop input format: "width:height:x:y" or - for no crop, for example: "100:200:0:0"
        // sendFrameMeta for recorder mp4
        m_server->start(m_serial, 27183, m_maxSize, m_bitRate, "-", sendFrameMeta);
    });

    updateShowSize(size());

    bool vertical = size().height() > size().width();
    updateStyleSheet(vertical);
}

VideoForm::~VideoForm()
{
    m_server->stop();
    // server must stop before decoder, because decoder block main thread
    m_decoder.stopDecode();
    delete m_server;
    if (m_recorder) {
        delete m_recorder;
    }
    m_frames.deInit();
    delete ui;
}

void VideoForm::initUI()
{
    QPixmap phone;
    if (phone.load(":/res/phone.png")) {
        m_widthHeightRatio = 1.0f * phone.width() / phone.height();
    }

    setAttribute(Qt::WA_DeleteOnClose);

    // 去掉标题栏
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    // 根据图片构造异形窗口
    setAttribute(Qt::WA_TranslucentBackground);

    setMouseTracking(true);
    ui->loadingWidget->setAttribute(Qt::WA_DeleteOnClose);
    ui->videoWidget->setMouseTracking(true);
    ui->videoWidget->hide();

    // 最后绘制，不设置最后绘制会影响父窗体异形异常（quickWidget的透明通道会形成穿透）
    ui->quickWidget->setAttribute(Qt::WA_AlwaysStackOnTop);
    // 背景透明
    ui->quickWidget->setClearColor(QColor(Qt::transparent));
}

void VideoForm::initSignals()
{
    connect(&m_fileHandler, &FileHandler::fileHandlerResult, this, [this](FileHandler::FILE_HANDLER_RESULT processResult){
        if (FileHandler::FAR_IS_RUNNING == processResult) {
            QMessageBox::warning(this, "QtScrcpy", tr("wait current file transfer to complete"), QMessageBox::Ok);
        }
        if (FileHandler::FAR_SUCCESS_EXEC == processResult) {
            QMessageBox::information(this, "QtScrcpy", tr("file transfer complete"), QMessageBox::Ok);
        }
        if (FileHandler::FAR_ERROR_EXEC == processResult) {
            QMessageBox::information(this, "QtScrcpy", tr("file transfer failed"), QMessageBox::Ok);
        }
    });
    connect(&m_inputConvert, &InputConvertGame::grabCursor, this, [this](bool grab){
#ifdef Q_OS_WIN32
        if(grab) {
            QRect rc(mapToGlobal(ui->videoWidget->pos())
                     , ui->videoWidget->size());
            RECT mainRect;
            mainRect.left = (LONG)rc.left();
            mainRect.right = (LONG)rc.right();
            mainRect.top = (LONG)rc.top();
            mainRect.bottom = (LONG)rc.bottom();
            ClipCursor(&mainRect);
        } else {
            ClipCursor(Q_NULLPTR);
        }
#endif
    });
    connect(m_server, &Server::serverStartResult, this, [this](bool success){
        if (success) {
            m_server->connectTo();
        } else {
            close();
        }
    });

    connect(m_server, &Server::connectToResult, this, [this](bool success, const QString &deviceName, const QSize &size){
        if (success) {
            float diff = m_startTimeCount.elapsed() / 1000.0f;
            qInfo(QString("server start finish in %1s").arg(diff).toStdString().c_str());


            // update ui
            setWindowTitle(deviceName);
            updateShowSize(size);

            // init recorder
            if (m_recorder) {
                m_recorder->setFrameSize(size);
            }

            // init decoder
            m_decoder.setDeviceSocket(m_server->getDeviceSocket());
            m_decoder.startDecode();

            // init controller
            m_inputConvert.setDeviceSocket(m_server->getDeviceSocket());
        }
    });

    connect(m_server, &Server::onServerStop, this, [this](){
        close();
        qDebug() << "server process stop";
    });

    connect(&m_decoder, &Decoder::onDecodeStop, this, [this](){
        close();
        qDebug() << "decoder thread stop";
    });

    // must be Qt::QueuedConnection, ui update must be main thread
    connect(&m_decoder, &Decoder::onNewFrame, this, [this](){
        if (ui->videoWidget->isHidden()) {
            ui->loadingWidget->close();
            ui->videoWidget->show();
        }
        m_frames.lock();
        const AVFrame *frame = m_frames.consumeRenderedFrame();
        //qDebug() << "widthxheight:" << frame->width << "x" << frame->height;
        updateShowSize(QSize(frame->width, frame->height));
        ui->videoWidget->setFrameSize(QSize(frame->width, frame->height));
        ui->videoWidget->updateTextures(frame->data[0], frame->data[1], frame->data[2], frame->linesize[0], frame->linesize[1], frame->linesize[2]);
        m_frames.unLock();
    },Qt::QueuedConnection);
}

void VideoForm::showToolFrom(bool show)
{
    if (!m_toolForm) {
        m_toolForm = new ToolForm(this, ToolForm::AP_OUTSIDE_RIGHT);
        m_toolForm->move(pos().x() + geometry().width(), pos().y() + 30);
    }
    m_toolForm->setVisible(show);
}

void VideoForm::updateStyleSheet(bool vertical)
{
    if (vertical) {
        setStyleSheet(R"(
                 #videoForm {
                     border-image: url(:/res/phone-v.png) 150px 142px 85px 142px;
                     border-width: 150px 142px 85px 142px;
                 }
                 )");
        layout()->setContentsMargins(10, 68, 12, 62);
    } else {
        setStyleSheet(R"(
                 #videoForm {
                     border-image: url(:/res/phone-h.png) 142px 85px 142px 150px;
                     border-width: 142px 85px 142px 150px;
                 }
                 )");
        layout()->setContentsMargins(68, 12, 62, 10);
    }
}

void VideoForm::updateShowSize(const QSize &newSize)
{
    if (frameSize != newSize) {
        frameSize = newSize;

        bool vertical = newSize.height() > newSize.width();
        QSize showSize = newSize;
        QDesktopWidget* desktop = QApplication::desktop();
        if (desktop) {
            QRect screenRect = desktop->availableGeometry();
            if (vertical) {
                showSize.setHeight(qMin(newSize.height(), screenRect.height() - 200));
                showSize.setWidth(showSize.height() * m_widthHeightRatio);
            } else {
                showSize.setWidth(qMin(newSize.width(), screenRect.width()/2));
                showSize.setHeight(showSize.width() * m_widthHeightRatio);
            }

            if (isFullScreen()) {
                switchFullScreen();
            }
            // 窗口居中
            move(screenRect.center() - QRect(0, 0, showSize.width(), showSize.height()).center());
        }

        // 减去标题栏高度 (mark:已经没有标题栏了)
        //int titleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);
        //showSize.setHeight(showSize.height() - titleBarHeight);

        if (showSize != size()) {
#ifdef Q_OS_OSX
            setFixedSize(showSize);
#else
            resize(showSize);
#endif
            updateStyleSheet(vertical);
        }
    }
}

void VideoForm::switchFullScreen()
{
    if (isFullScreen()) {
        showNormal();
#ifdef Q_OS_OSX
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        show();
#endif
        updateStyleSheet(height() > width());
        showToolFrom(true);
#ifdef Q_OS_WIN32
        ::SetThreadExecutionState(ES_CONTINUOUS);
#endif
    } else {
        // mac fullscreen must show title bar
#ifdef Q_OS_OSX
        setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
#endif
        showToolFrom(false);
        layout()->setContentsMargins(0, 0, 0, 0);
        showFullScreen();

        // 全屏状态禁止电脑休眠、息屏
#ifdef Q_OS_WIN32
        ::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#endif
    }
}

void VideoForm::postGoMenu()
{
    postKeyCodeClick(AKEYCODE_MENU);
}

void VideoForm::postGoBack()
{
    postKeyCodeClick(AKEYCODE_BACK);
}

void VideoForm::postAppSwitch()
{
    postKeyCodeClick(AKEYCODE_APP_SWITCH);
}

void VideoForm::postPower()
{
    postKeyCodeClick(AKEYCODE_POWER);
}

void VideoForm::postVolumeUp()
{
    postKeyCodeClick(AKEYCODE_VOLUME_UP);
}

void VideoForm::postVolumeDown()
{
    postKeyCodeClick(AKEYCODE_VOLUME_DOWN);
}

void VideoForm::postTurnOn()
{
    ControlEvent* controlEvent = new ControlEvent(ControlEvent::CET_COMMAND);
    if (!controlEvent) {
        return;
    }
    controlEvent->setCommandEventData(CONTROL_EVENT_COMMAND_BACK_OR_SCREEN_ON);
    m_inputConvert.sendControlEvent(controlEvent);
}

void VideoForm::postTextInput(const QString& text)
{
    ControlEvent* controlEvent = new ControlEvent(ControlEvent::CET_TEXT);
    if (!controlEvent) {
        return;
    }
    controlEvent->setTextEventData(text);
    m_inputConvert.sendControlEvent(controlEvent);
}

void VideoForm::postGoHome()
{
    postKeyCodeClick(AKEYCODE_HOME);
}

void VideoForm::postKeyCodeClick(AndroidKeycode keycode)
{
    ControlEvent* controlEventDown = new ControlEvent(ControlEvent::CET_KEYCODE);
    if (!controlEventDown) {
        return;
    }
    controlEventDown->setKeycodeEventData(AKEY_EVENT_ACTION_DOWN, keycode, AMETA_NONE);
    m_inputConvert.sendControlEvent(controlEventDown);

    ControlEvent* controlEventUp = new ControlEvent(ControlEvent::CET_KEYCODE);
    if (!controlEventUp) {
        return;
    }
    controlEventUp->setKeycodeEventData(AKEY_EVENT_ACTION_UP, keycode, AMETA_NONE);
    m_inputConvert.sendControlEvent(controlEventUp);
}

void VideoForm::mousePressEvent(QMouseEvent *event)
{
    if (ui->videoWidget->geometry().contains(event->pos())) {
        event->setLocalPos(ui->videoWidget->mapFrom(this, event->localPos().toPoint()));
        m_inputConvert.mouseEvent(event, ui->videoWidget->frameSize(), ui->videoWidget->size());
    } else {
        if (event->button() == Qt::LeftButton) {
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
}

void VideoForm::mouseReleaseEvent(QMouseEvent *event)
{
    if (ui->videoWidget->geometry().contains(event->pos())) {
        event->setLocalPos(ui->videoWidget->mapFrom(this, event->localPos().toPoint()));
        m_inputConvert.mouseEvent(event, ui->videoWidget->frameSize(), ui->videoWidget->size());
    }
}

void VideoForm::mouseMoveEvent(QMouseEvent *event)
{    
    if (ui->videoWidget->geometry().contains(event->pos())) {
        event->setLocalPos(ui->videoWidget->mapFrom(this, event->localPos().toPoint()));
        m_inputConvert.mouseEvent(event, ui->videoWidget->frameSize(), ui->videoWidget->size());
    } else {
        if (event->buttons() & Qt::LeftButton) {
            move(event->globalPos() - m_dragPosition);
            event->accept();
        }
    }
}

void VideoForm::wheelEvent(QWheelEvent *event)
{
    if (ui->videoWidget->geometry().contains(event->pos())) {
        QPointF pos = ui->videoWidget->mapFrom(this, event->pos());
        /*
        QWheelEvent(const QPointF &pos, const QPointF& globalPos, int delta,
                Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers,
                Qt::Orientation orient = Qt::Vertical);
        */
        QWheelEvent wheelEvent(pos, event->globalPosF(), event->delta(),
                               event->buttons(), event->modifiers(), event->orientation());
        m_inputConvert.wheelEvent(&wheelEvent, ui->videoWidget->frameSize(), ui->videoWidget->size());
    }
}

void VideoForm::keyPressEvent(QKeyEvent *event)
{
    if (Qt::Key_Escape == event->key()
            && !event->isAutoRepeat()
            && isFullScreen()) {
        switchFullScreen();
    }
    //qDebug() << "keyPressEvent" << event->isAutoRepeat();
    m_inputConvert.keyEvent(event, ui->videoWidget->frameSize(), ui->videoWidget->size());
}

void VideoForm::keyReleaseEvent(QKeyEvent *event)
{
    //qDebug() << "keyReleaseEvent" << event->isAutoRepeat();
    m_inputConvert.keyEvent(event, ui->videoWidget->frameSize(), ui->videoWidget->size());
}

void VideoForm::paintEvent(QPaintEvent *paint)
{
    Q_UNUSED(paint);
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void VideoForm::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    showToolFrom();
}

void VideoForm::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void VideoForm::dragMoveEvent(QDragMoveEvent *event)
{
    Q_UNUSED(event);
}

void VideoForm::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
}

void VideoForm::dropEvent(QDropEvent *event)
{
    const QMimeData* qm = event->mimeData();
    QString file = qm->urls()[0].toLocalFile();
    QFileInfo fileInfo(file);

    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "QtScrcpy", tr("file does not exist"), QMessageBox::Ok);
        return;
    }

    if (fileInfo.isFile() && fileInfo.suffix() == "apk") {
        m_fileHandler.installApkRequest(m_serial, file);
        return;
    }
    m_fileHandler.pushFileRequest(m_serial, file);
}
