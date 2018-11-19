#include <QFile>
#include <QTime>
#include <QKeyEvent>

#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    //setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);

    connect(&m_adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        QString log = "";
        switch (processResult) {
        case AdbProcess::AER_SUCCESS_START:
            log = "adb run";
            break;
        case AdbProcess::AER_ERROR_EXEC:
            log = m_adb.getErrorOut();
            break;
        case AdbProcess::AER_ERROR_MISSING_BINARY:
            log = "adb not find";
            break;
        case AdbProcess::AER_SUCCESS_EXEC:
            log = m_adb.getStdOut();
            break;
        }
        if (!log.isEmpty()) {
            outLog(log);
        }
    });
}

Dialog::~Dialog()
{    
    on_stopServerBtn_clicked();
    delete ui;
}

void Dialog::on_updateDevice_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    outLog("update devices...");
    m_adb.execute("", QStringList() << "devices");
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
    if (checkAdbRun()) {
        return;
    }
    outLog("wireless connect...");
    QStringList adbArgs;
    adbArgs << "connect";
    adbArgs << ui->deviceIpEdt->text().trimmed();
    m_adb.execute("", adbArgs);
}

void Dialog::on_startAdbdBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    outLog("start devices adbd...");
    // adb tcpip 5555
    QStringList adbArgs;
    adbArgs << "tcpip";
    adbArgs << "5555";
    m_adb.execute("", adbArgs);
}

void Dialog::outLog(const QString &log)
{
    ui->outEdit->append(log);
}

bool Dialog::checkAdbRun()
{
    if (m_adb.isRuning()) {
        outLog("wait for the end of the current command to run");
    }
    return m_adb.isRuning();
}
