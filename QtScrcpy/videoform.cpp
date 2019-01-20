#include <QDesktopWidget>
#include <QMouseEvent>
#include <QTimer>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#ifdef Q_OS_WIN32
#include <Windows.h>
#endif

#include "videoform.h"
#include "ui_videoform.h"
#include "iconhelper.h"
#include "toolform.h"

VideoForm::VideoForm(const QString& serial, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::videoForm),
    m_serial(serial)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);    

    QPixmap phone;
    if (phone.load(":/res/phone.png")) {
        m_widthHeightRatio = 1.0f * phone.width() / phone.height();
    }

    // 去掉标题栏
    setWindowFlags(Qt::FramelessWindowHint);
    // 根据图片构造异形窗口
    setAttribute(Qt::WA_TranslucentBackground);

    setMouseTracking(true);
    ui->videoWidget->setMouseTracking(true);

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

    m_server = new Server();
    m_frames.init();
    m_decoder.setFrames(&m_frames);

    connect(m_server, &Server::serverStartResult, this, [this](bool success){
        if (success) {
            m_server->connectTo();            
        }
    });

    connect(m_server, &Server::connectToResult, this, [this](bool success, const QString &deviceName, const QSize &size){
        if (success) {
            // update ui
            setWindowTitle(deviceName);
            updateShowSize(size);            

            // init decode
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
    QObject::connect(&m_decoder, &Decoder::onNewFrame, this, [this](){
        m_frames.lock();        
        const AVFrame *frame = m_frames.consumeRenderedFrame();
        //qDebug() << "widthxheight:" << frame->width << "x" << frame->height;
        updateShowSize(QSize(frame->width, frame->height));
        ui->videoWidget->setFrameSize(QSize(frame->width, frame->height));
        ui->videoWidget->updateTextures(frame->data[0], frame->data[1], frame->data[2], frame->linesize[0], frame->linesize[1], frame->linesize[2]);
        m_frames.unLock();
    },Qt::QueuedConnection);

    // fix: macos cant recv finished signel, timer is ok
    QTimer::singleShot(0, this, [this](){
        // support 480p 720p 1080p
        //m_server->start("P7C0218510000537", 27183, 0, 8000000, "");
        //m_server->start("P7C0218510000537", 27183, 1080, 8000000, "");

        // only one devices, serial can be null
        m_server->start(m_serial, 27183, 720, 8000000, "");

        // support wireless connect
        //m_server->start("192.168.0.174:5555", 27183, 720, 8000000, "");
    });

    updateShowSize(size());
    initStyle();

    bool vertical = size().height() > size().width();
    updateStyleSheet(vertical);

    ToolForm* mw = new ToolForm(this, ToolForm::AP_OUTSIDE_RIGHT);
    mw->move(pos().x() + geometry().width(), pos().y() + 30);
    mw->show();
}

VideoForm::~VideoForm()
{
    m_server->stop();
    m_decoder.stopDecode();
    delete m_server;
    m_frames.deInit();
    delete ui;
}

void VideoForm::initStyle()
{
    //IconHelper::Instance()->SetIcon(ui->fullScrcenbtn, QChar(0xf0b2), 13);
    //IconHelper::Instance()->SetIcon(ui->returnBtn, QChar(0xf104), 15);
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
                showSize.setWidth(qMin(newSize.width(), screenRect.width()));
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
            resize(showSize);
            updateStyleSheet(vertical);
        }
    }
}

void VideoForm::switchFullScreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
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
        QPoint pos = ui->videoWidget->mapFrom(this, event->pos());
        QWheelEvent wheelEvent(pos, event->globalPosF(), event->pixelDelta(),
                               event->angleDelta(), event->buttons(), event->modifiers(),
                               event->phase(), event->inverted(), event->source());
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
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void VideoForm::on_fullScrcenbtn_clicked()
{
    switchFullScreen();    
}

void VideoForm::on_returnBtn_clicked()
{

}
