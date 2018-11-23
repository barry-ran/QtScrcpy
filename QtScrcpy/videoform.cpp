#include <QDesktopWidget>
#include <QMouseEvent>
#ifdef Q_OS_WIN32
#include <Windows.h>
#endif

#include "videoform.h"
#include "ui_videoform.h"

VideoForm::VideoForm(const QString& serial, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::videoForm),
    m_serial(serial)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    setMouseTracking(true);
    ui->videoWidget->setMouseTracking(true);

    connect(&m_inputConvert, &InputConvertGame::grabCursor, this, [this](bool grab){
#ifdef Q_OS_WIN32
        if(grab) {
            RECT mainRect;
            mainRect.left = (LONG)this->geometry().left();
            mainRect.right = (LONG)this->geometry().right();
            mainRect.top = (LONG)this->geometry().top();
            mainRect.bottom = (LONG)this->geometry().bottom();
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

    // support 480p 720p 1080p
    //m_server->start("P7C0218510000537", 27183, 0, 8000000, "");
    //m_server->start("P7C0218510000537", 27183, 1080, 8000000, "");

    // only one devices, serial can be null
    m_server->start(m_serial, 27183, 720, 8000000, "");

    // support wireless connect
    //m_server->start("192.168.0.174:5555", 27183, 720, 8000000, "");
}

VideoForm::~VideoForm()
{
    m_server->stop();
    m_decoder.stopDecode();
    delete m_server;
    m_frames.deInit();
    delete ui;
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
                showSize.setHeight(qMin(newSize.height(), screenRect.height()));
                showSize.setWidth(showSize.height()/2);
            } else {
                showSize.setWidth(qMin(newSize.width(), screenRect.width()));
                showSize.setHeight(showSize.width()/2);
            }

            // 窗口居中            
            move(screenRect.center() - QRect(0, 0, showSize.width(), showSize.height()).center());
        }

        int titleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);
        // 减去标题栏高度
        showSize.setHeight(showSize.height() - titleBarHeight);
        if (showSize != size()) {
            resize(showSize);
        }
    }
}

void VideoForm::mousePressEvent(QMouseEvent *event)
{
    m_inputConvert.mouseEvent(event, ui->videoWidget->frameSize(), size());
}

void VideoForm::mouseReleaseEvent(QMouseEvent *event)
{
    m_inputConvert.mouseEvent(event, ui->videoWidget->frameSize(), size());
}

void VideoForm::mouseMoveEvent(QMouseEvent *event)
{
    m_inputConvert.mouseEvent(event, ui->videoWidget->frameSize(), size());
}

void VideoForm::wheelEvent(QWheelEvent *event)
{
    m_inputConvert.wheelEvent(event, ui->videoWidget->frameSize(), size());

}

void VideoForm::keyPressEvent(QKeyEvent *event)
{
    //qDebug() << "keyPressEvent" << event->isAutoRepeat();
    m_inputConvert.keyEvent(event, ui->videoWidget->frameSize(), size());
}

void VideoForm::keyReleaseEvent(QKeyEvent *event)
{
    //qDebug() << "keyReleaseEvent" << event->isAutoRepeat();
    m_inputConvert.keyEvent(event, ui->videoWidget->frameSize(), size());
}
