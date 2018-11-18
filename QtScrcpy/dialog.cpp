#include <QFile>
#include <QTime>
#include <QKeyEvent>

#include "dialog.h"
#include "ui_dialog.h"
#include "adbprocess.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    //setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
}

Dialog::~Dialog()
{    
    on_stopServerBtn_clicked();
    delete ui;
}

void Dialog::on_adbProcess_clicked()
{
    AdbProcess* adb = new AdbProcess();
    connect(adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        if (AdbProcess::AER_SUCCESS_START != processResult) {
            sender()->deleteLater();
        }
    });
    adb->execute("", QStringList() << "devices");
    //adb->setShowTouchesEnabled("P7C0218510000537", true);
}

void Dialog::on_startServerBtn_clicked()
{
    if (!m_videoForm) {
        m_videoForm = new VideoForm();
    }
    m_videoForm->show();
}

void Dialog::on_stopServerBtn_clicked()
{    
    if (m_videoForm) {
        m_videoForm->close();
    }
}
