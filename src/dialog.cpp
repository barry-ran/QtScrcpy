#include <QFile>
#include <QTime>

#include "dialog.h"
#include "ui_dialog.h"
#include "adbprocess.h"
#include "yuvglwidget.h"

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

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    w2 = new YUVGLWidget(this);
    w2->resize(ui->imgLabel->size());
    w2->move(230, 20);

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
        w2->setFrameSize(frame->width, frame->height);
        w2->setYPixels(frame->data[0], frame->linesize[0]);
        w2->setUPixels(frame->data[1], frame->linesize[1]);
        w2->setVPixels(frame->data[2], frame->linesize[2]);
        w2->update();

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
