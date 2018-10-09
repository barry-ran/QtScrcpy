#include "dialog.h"
#include "ui_dialog.h"
#include "adbprocess.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
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
