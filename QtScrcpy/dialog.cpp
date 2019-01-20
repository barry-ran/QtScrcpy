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
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

    connect(&m_adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        QString log = "";
        bool newLine = true;

        switch (processResult) {
        case AdbProcess::AER_SUCCESS_START:
            log = "adb run";
            newLine = false;
            break;
        case AdbProcess::AER_ERROR_EXEC:
            //log = m_adb.getErrorOut();
            break;
        case AdbProcess::AER_ERROR_MISSING_BINARY:
            log = "adb not find";
            break;
        case AdbProcess::AER_SUCCESS_EXEC:
            //log = m_adb.getStdOut();
            QStringList args = m_adb.arguments();
            if (args.contains("devices")) {
                QStringList devices = m_adb.getDevicesSerialFromStdOut();
                if (!devices.isEmpty()) {
                    ui->serialEdt->setText(devices.at(0));
                }
            } else if (args.contains("show") && args.contains("wlan0")) {
                QString ip = m_adb.getDeviceIPFromStdOut();
                if (!ip.isEmpty()) {
                    ui->deviceIpEdt->setText(ip);
                }
            }
            break;
        }
        if (!log.isEmpty()) {
            outLog(log, newLine);
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
    outLog("update devices...", false);
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
    QString addr = ui->deviceIpEdt->text().trimmed();
    if (!ui->devicePortEdt->text().isEmpty()) {
        addr += ":";
        addr += ui->devicePortEdt->text().trimmed();
    }
    outLog("wireless connect...", false);
    QStringList adbArgs;
    adbArgs << "connect";
    adbArgs << addr;
    m_adb.execute("", adbArgs);
}

void Dialog::on_startAdbdBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    outLog("start devices adbd...", false);
    // adb tcpip 5555
    QStringList adbArgs;
    adbArgs << "tcpip";
    adbArgs << "5555";
    m_adb.execute(ui->serialEdt->text().trimmed(), adbArgs);
}

void Dialog::outLog(const QString &log, bool newLine)
{
    ui->outEdit->append(log);
    if (newLine) {
        ui->outEdit->append("<br/>");
    }
}

bool Dialog::checkAdbRun()
{
    if (m_adb.isRuning()) {
        outLog("wait for the end of the current command to run");
    }
    return m_adb.isRuning();
}

void Dialog::on_getIPBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }

    outLog("get ip...", false);
    // adb -s P7C0218510000537 shell ifconfig wlan0
    // or
    // adb -s P7C0218510000537 shell ip -f inet addr show wlan0
    QStringList adbArgs;
    adbArgs << "shell";
    adbArgs << "ip";
    adbArgs << "-f";
    adbArgs << "inet";
    adbArgs << "addr";
    adbArgs << "show";
    adbArgs << "wlan0";
    m_adb.execute(ui->serialEdt->text().trimmed(), adbArgs);
}
