#include "dialog.h"
#include "ui_dialog.h"
#include "adbprocess.h"
#include "glyuvwidget.h"


Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    w = new GLYuvWidget(this);
    w->resize(ui->imgLabel->size());
    w->move(230, 20);

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
            w->setVideoSize(frame->width, frame->height);
            /*
            if (!prepare_for_frame(screen, new_frame_size)) {
                mutex_unlock(frames->mutex);
                return SDL_FALSE;
            }
            */
            w->updateTexture(frame->data[0], frame->data[1], frame->data[2], frame->linesize[0], frame->linesize[1], frame->linesize[2]);
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
