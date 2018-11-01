#include <QFile>
#include <QTime>

#include "dialog.h"
#include "ui_dialog.h"
#include "adbprocess.h"
#include "yuvglwidget.h"
#include "qyuvopenglwidget.h"

void saveAVFrame_YUV_ToTempFile(const AVFrame *pFrame)
{
    int t_frameWidth = pFrame->width;
    int t_frameHeight = pFrame->height;
    int t_yPerRowBytes = pFrame->linesize[0];
    int t_uPerRowBytes = pFrame->linesize[1];
    int t_vPerRowBytes = pFrame->linesize[2];
    qDebug()<<"robin:saveAVFrame_YUV_ToTempFile info:"<<t_frameWidth<<t_frameHeight<<"||"<<t_yPerRowBytes<<t_uPerRowBytes<<t_vPerRowBytes;
    QFile t_file("E:\\receive_Frame.yuv");
    t_file.open(QIODevice::WriteOnly | QIODevice::Append);
    //t_file.write((char *)pFrame->data[0],t_frameWidth * t_frameHeight);
    //t_file.write((char *)pFrame->data[1],(t_frameWidth/2) * t_frameHeight / 2);
    //t_file.write((char *)pFrame->data[2],(t_frameWidth/2) * t_frameHeight / 2);

    for(int i = 0;i< t_frameHeight ;i++)
    {
        t_file.write((char*)(pFrame->data[0]+i*t_yPerRowBytes),t_frameWidth);
    }

    for(int i = 0;i< t_frameHeight/2 ;i++)
    {
        t_file.write((char*)(pFrame->data[1]+i*t_uPerRowBytes),t_frameWidth/2);
    }

    for(int i = 0;i< t_frameHeight/2 ;i++)
    {
        t_file.write((char*)(pFrame->data[2]+i*t_vPerRowBytes),t_frameWidth/2);
    }

    t_file.flush();

}
#define OPENGL_EX
Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()
                   | Qt::WindowMaximizeButtonHint
                   | Qt::WindowMinimizeButtonHint);
#ifdef OPENGL_EX
    w = new QYUVOpenGLWidget(this);
    ui->verticalLayout->addWidget(w);
#else
    w2 = new YUVGLWidget(this);
    ui->verticalLayout->addWidget(w2);
#endif


    Decoder::init();

    frames.init();
    decoder.setFrames(&frames);

    server = new Server();
    connect(server, &Server::serverStartResult, this, [this](bool success){
        if (success) {
            server->connectTo();
        }
    });

    connect(server, &Server::connectToResult, this, [this](bool success){
        if (success) {
            decoder.setDeviceSocket(server->getDeviceSocketByThread(&decoder));
            decoder.startDecode();
        }
    });

    // must be Qt::QueuedConnection, ui update must be main thread
    QObject::connect(&decoder, &Decoder::newFrame, this, [this](){
        frames.lock();
        const AVFrame *frame = frames.consumeRenderedFrame();
        //saveAVFrame_YUV_ToTempFile(frame);
#ifdef OPENGL_EX
        w->setFrameSize(QSize(frame->width, frame->height));
        w->updateTextures(frame->data[0], frame->data[1], frame->data[2], frame->linesize[0], frame->linesize[1], frame->linesize[2]);
#else
        w2->setFrameSize(QSize(frame->width, frame->height));
        w2->updateTextures(frame->data[0], frame->data[1], frame->data[2], frame->linesize[0], frame->linesize[1], frame->linesize[2]);
#endif
        frames.unLock();
    },Qt::QueuedConnection);
}

Dialog::~Dialog()
{
    Decoder::deInit();
    frames.deInit();
    delete ui;
}

void Dialog::on_adbProcess_clicked()
{
    AdbProcess* adb = new AdbProcess();
    connect(adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        sender()->deleteLater();
    });
    adb->execute("", QStringList() << "devices");
}

void Dialog::on_startServerBtn_clicked()
{
    server->start("P7C0218510000537", 27183, 0, 8000000, "");
    //server->start("P7CDU17C23010875", 27183, 0, 8000000, "");
}

void Dialog::on_stopServerBtn_clicked()
{
    decoder.stopDecode();
    server->stop();
}
