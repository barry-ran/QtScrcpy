#include "dialog.h"
#include "ui_dialog.h"
#include "adbprocess.h"
#include "glyuvwidget.h"


Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    GLYuvWidget* w = new GLYuvWidget(this);
    w->resize(ui->imgLabel->size());
    w->move(230, 20);
    Decoder::init();

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
    QObject::connect(&decoder, &Decoder::getOneFrame,w,&GLYuvWidget::slotShowYuv,
                     Qt::QueuedConnection);

    /*
    // must be Qt::QueuedConnection, ui update must be main thread
    connect(&decoder, &Decoder::getOneImage, this, [this](QImage img){
        qDebug() << "getOneImage";

        return;
        //18% cpu
        // 将图像按比例缩放成和窗口一样大小
        QImage img2 = img.scaled(ui->imgLabel->size(), Qt::IgnoreAspectRatio);
        ui->imgLabel->setPixmap(QPixmap::fromImage(img2));
        //24% cpu

    }, Qt::QueuedConnection);
    */
}

Dialog::~Dialog()
{
    Decoder::deInit();
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
