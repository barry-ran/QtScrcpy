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

void Dialog::on_updateDevice_clicked()
{
    AdbProcess* adb = new AdbProcess();
    connect(adb, &AdbProcess::adbProcessResult, this, [this, adb](AdbProcess::ADB_EXEC_RESULT processResult){
        if (AdbProcess::AER_SUCCESS_EXEC == processResult) {
            ui->outEdit->append(adb->getDevicesSerialFromStdOut().join("*"));
        }
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
        m_videoForm = new VideoForm(ui->serialEdt->text().trimmed());
    }
    m_videoForm->show();
}

void Dialog::on_stopServerBtn_clicked()
{    
    if (m_videoForm) {
        m_videoForm->close();
    }
}

void Dialog::on_wirelessConnectBtn_clicked()
{
    AdbProcess* adb = new AdbProcess();
    connect(adb, &AdbProcess::adbProcessResult, this, [this, adb](AdbProcess::ADB_EXEC_RESULT processResult){
        if (AdbProcess::AER_SUCCESS_EXEC == processResult) {
            ui->outEdit->append(adb->getStdOut());
        }
        if (AdbProcess::AER_SUCCESS_START != processResult) {
            sender()->deleteLater();
        }
    });

    //adb connect 172.16.8.197:5555
    QStringList adbArgs;
    adbArgs << "connect";
    adbArgs << ui->deviceIpEdt->text().trimmed();
    adb->execute("", adbArgs);
}
