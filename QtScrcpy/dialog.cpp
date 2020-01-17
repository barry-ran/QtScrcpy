#include <QFile>
#include <QTime>
#include <QKeyEvent>
#include <QFileDialog>
#include <QTimer>

#include "dialog.h"
#include "ui_dialog.h"
#include "device.h"
#include "videoform.h"
#include "keymap.h"
#include "config.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{    
    ui->setupUi(this);
    initUI();

    connect(&m_adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        QString log = "";
        bool newLine = true;

        switch (processResult) {
        case AdbProcess::AER_ERROR_START:
            break;
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
                ui->serialBox->clear();
                for (auto& item : devices) {
                    ui->serialBox->addItem(item);
                }
            } else if (args.contains("show") && args.contains("wlan0")) {
                QString ip = m_adb.getDeviceIPFromStdOut();
                if (!ip.isEmpty()) {
                    ui->deviceIpEdt->setText(ip);
                }
            } else if (args.contains("ifconfig") && args.contains("wlan0")) {
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
    m_deviceManage.disconnectAllDevice();
    delete ui;
}

void Dialog::initUI()
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

    ui->bitRateBox->addItem("2000000");
    ui->bitRateBox->addItem("6000000");
    ui->bitRateBox->addItem("8000000");
    ui->bitRateBox->addItem("10000000");
    ui->bitRateBox->setCurrentIndex(2);

    ui->videoSizeBox->addItem("480");
    ui->videoSizeBox->addItem("720");
    ui->videoSizeBox->addItem("1080");
    ui->videoSizeBox->addItem("native");
    ui->videoSizeBox->setCurrentIndex(1);

    ui->formatBox->addItem("mp4");
    ui->formatBox->addItem("mkv");

#ifndef Q_OS_WIN32
    // game only windows
    ui->gameCheck->setEnabled(false);
#endif

    ui->recordPathEdt->setText(Config::getInstance().getRecordPath());
}

void Dialog::execAdbCmd()
{
    if (checkAdbRun()) {
        return;
    }
    QString cmd = ui->adbCommandEdt->text().trimmed();
    outLog("adb " + cmd, false);
    m_adb.execute("", cmd.split(" ", QString::SkipEmptyParts));
}

QString Dialog::getGameScript(const QString& fileName)
{
    QFile loadFile(KeyMap::getKeyMapPath() + "/" + fileName);
    if(!loadFile.open(QIODevice::ReadOnly))
    {
        outLog("open file failed:" + fileName, true);
        return "";
    }

    QString ret = loadFile.readAll();
    loadFile.close();
    return ret;
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
    outLog("start server...", false);

    QString absFilePath;
    QString fileDir(ui->recordPathEdt->text().trimmed());
    if (!fileDir.isEmpty()) {
        QDateTime dateTime = QDateTime::currentDateTime();
        QString fileName = dateTime.toString("_yyyyMMdd_hhmmss_zzz");
        QString ext = ui->formatBox->currentText().trimmed();
        fileName = windowTitle() + fileName + "." + ext;
        QDir dir(fileDir);
        absFilePath = dir.absoluteFilePath(fileName);
    }

    quint32 bitRate = ui->bitRateBox->currentText().trimmed().toUInt();
    // this is ok that "native" toUshort is 0
    quint16 videoSize = ui->videoSizeBox->currentText().trimmed().toUShort();
    Device::DeviceParams params;
    params.serial = ui->serialBox->currentText().trimmed();
    params.maxSize = videoSize;
    params.bitRate = bitRate;
    // on devices with Android >= 10, the capture frame rate can be limited
    params.maxFps = Config::getInstance().getMaxFps();
    params.recordFileName = absFilePath;
    params.closeScreen = ui->closeScreenCheck->isChecked();
    params.useReverse = ui->useReverseCheck->isChecked();
    params.display = !ui->notDisplayCheck->isChecked();
    if (ui->gameCheck->isChecked()) {
        if (ui->gameBox->currentText().isEmpty()) {
            outLog("no keymap script selected", true);
        } else {
            params.gameScript = getGameScript(ui->gameBox->currentText());
        }
    }
    m_deviceManage.connectDevice(params);

/*
        if (ui->alwaysTopCheck->isChecked() && m_device->getVideoForm()) {
            m_device->getVideoForm()->staysOnTop();
        }         
    */
}

void Dialog::on_stopServerBtn_clicked()
{    
    if (m_deviceManage.disconnectDevice(ui->serialBox->currentText().trimmed())) {
        outLog("stop server");
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
    } else if (!ui->devicePortEdt->placeholderText().isEmpty()) {
        addr += ":";
        addr += ui->devicePortEdt->placeholderText().trimmed();
    } else {
        outLog("error: device port is null", false);
        return;
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
    m_adb.execute(ui->serialBox->currentText().trimmed(), adbArgs);
}

void Dialog::outLog(const QString &log, bool newLine)
{
    // avoid sub thread update ui
    QString backLog = log;
    QTimer::singleShot(0, this, [this, backLog, newLine](){
        ui->outEdit->append(backLog);
        if (newLine) {
            ui->outEdit->append("<br/>");
        }
    });
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
#if 0
    adbArgs << "shell";
    adbArgs << "ip";
    adbArgs << "-f";
    adbArgs << "inet";
    adbArgs << "addr";
    adbArgs << "show";
    adbArgs << "wlan0";
#else
    adbArgs << "shell";
    adbArgs << "ifconfig";
    adbArgs << "wlan0";
#endif
    m_adb.execute(ui->serialBox->currentText().trimmed(), adbArgs);
}

void Dialog::on_wirelessDisConnectBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    QString addr = ui->deviceIpEdt->text().trimmed();
    outLog("wireless disconnect...", false);
    QStringList adbArgs;
    adbArgs << "disconnect";
    adbArgs << addr;
    m_adb.execute("", adbArgs);
}

void Dialog::on_selectRecordPathBtn_clicked()
{
    QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;
    QString directory = QFileDialog::getExistingDirectory(this,
                                                          tr("select path"),
                                                          "",
                                                          options);
    ui->recordPathEdt->setText(directory);
}

void Dialog::on_recordPathEdt_textChanged(const QString &arg1)
{
    Config::getInstance().setRecordPath(arg1);
    ui->recordPathEdt->setToolTip(arg1.trimmed());
    ui->notDisplayCheck->setCheckable(!arg1.trimmed().isEmpty());
}

void Dialog::on_adbCommandBtn_clicked()
{
    execAdbCmd();
}

void Dialog::on_stopAdbBtn_clicked()
{
    m_adb.kill();
}

void Dialog::on_clearOut_clicked()
{    
    ui->outEdit->clear();
}

void Dialog::on_stopAllServerBtn_clicked()
{
    m_deviceManage.disconnectAllDevice();
}

void Dialog::on_refreshGameScriptBtn_clicked()
{
    ui->gameBox->clear();
    QDir dir(KeyMap::getKeyMapPath());
    if (!dir.exists()) {
        outLog("keymap directory not find", true);
        return;
    }
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QFileInfoList list = dir.entryInfoList();
    QFileInfo fileInfo;
    int size = list.size();
    for (int i = 0; i < size; ++i) {
        fileInfo = list.at(i);
        ui->gameBox->addItem(fileInfo.fileName());
    }
}

void Dialog::on_applyScriptBtn_clicked()
{
    m_deviceManage.updateScript(getGameScript(ui->gameBox->currentText()));
}

void Dialog::on_gameCheck_clicked(bool checked)
{
    if (checked) {
        on_refreshGameScriptBtn_clicked();
    }
}
