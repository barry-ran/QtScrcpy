#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QKeyEvent>
#include <QTime>
#include <QTimer>

#include "config.h"
#include "device.h"
#include "dialog.h"
#include "keymap.h"
#include "ui_dialog.h"
#include "videoform.h"

Dialog::Dialog(QWidget *parent) : QDialog(parent), ui(new Ui::Dialog)
{
    ui->setupUi(this);
    initUI();

    connect(&m_adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult) {
        QString log = "";
        bool newLine = true;
        QStringList args = m_adb.arguments();

        switch (processResult) {
        case AdbProcess::AER_ERROR_START:
            break;
        case AdbProcess::AER_SUCCESS_START:
            log = "adb run";
            newLine = false;
            break;
        case AdbProcess::AER_ERROR_EXEC:
            //log = m_adb.getErrorOut();
            if (args.contains("ifconfig") && args.contains("wlan0")) {
                getIPbyIp();
            }
            break;
        case AdbProcess::AER_ERROR_MISSING_BINARY:
            log = "adb not find";
            break;
        case AdbProcess::AER_SUCCESS_EXEC:
            //log = m_adb.getStdOut();
            if (args.contains("devices")) {
                QStringList devices = m_adb.getDevicesSerialFromStdOut();
                ui->serialBox->clear();
                for (auto &item : devices) {
                    ui->serialBox->addItem(item);
                }
            } else if (args.contains("show") && args.contains("wlan0")) {
                QString ip = m_adb.getDeviceIPFromStdOut();
                if (ip.isEmpty()) {
                    log = "ip not find, connect to wifi?";
                    break;
                }
                ui->deviceIpEdt->setText(ip);
            } else if (args.contains("ifconfig") && args.contains("wlan0")) {
                QString ip = m_adb.getDeviceIPFromStdOut();
                if (ip.isEmpty()) {
                    log = "ip not find, connect to wifi?";
                    break;
                }
                ui->deviceIpEdt->setText(ip);
            } else if (args.contains("ip -o a")) {
                QString ip = m_adb.getDeviceIPByIpFromStdOut();
                if (ip.isEmpty()) {
                    log = "ip not find, connect to wifi?";
                    break;
                }
                ui->deviceIpEdt->setText(ip);
            }
            break;
        }
        if (!log.isEmpty()) {
            outLog(log, newLine);
        }
    });

    m_hideIcon = new QSystemTrayIcon();
    m_hideIcon->setIcon(QIcon(":/image/tray/logo.png"));
    m_menu = new QMenu();
    m_quit = new QAction();
    m_showWindow = new QAction();;
    m_showWindow->setText(tr("show"));
    m_quit->setText(tr("quit"));
    m_menu->addAction(m_showWindow);
    m_menu->addAction(m_quit);
    m_hideIcon->setContextMenu(m_menu);
    connect(m_showWindow, &QAction::triggered, this, &Dialog::slotShow);
    connect(m_quit, SIGNAL(triggered()), this, SLOT(close()));
    connect(m_hideIcon, &QSystemTrayIcon::activated,this,&Dialog::slotActivated);
}

Dialog::~Dialog()
{
    m_deviceManage.disconnectAllDevice();
    delete ui;
}

void Dialog::initUI()
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);

    ui->bitRateBox->addItem("2000000");
    ui->bitRateBox->addItem("6000000");
    ui->bitRateBox->addItem("8000000");
    ui->bitRateBox->addItem("10000000");
    ui->bitRateBox->addItem("20000000");
    ui->bitRateBox->addItem("50000000");
    ui->bitRateBox->addItem("100000000");
    ui->bitRateBox->addItem("200000000");
    ui->bitRateBox->setCurrentIndex(Config::getInstance().getBitRateIndex());

    ui->maxSizeBox->addItem("640");
    ui->maxSizeBox->addItem("720");
    ui->maxSizeBox->addItem("1080");
    ui->maxSizeBox->addItem("1280");
    ui->maxSizeBox->addItem("1920");
    ui->maxSizeBox->addItem(tr("original"));
    ui->maxSizeBox->setCurrentIndex(Config::getInstance().getMaxSizeIndex());

    ui->formatBox->addItem("mp4");
    ui->formatBox->addItem("mkv");
    ui->formatBox->setCurrentIndex(Config::getInstance().getRecordFormatIndex());

    ui->lockOrientationBox->addItem(tr("no lock"));
    ui->lockOrientationBox->addItem("0");
    ui->lockOrientationBox->addItem("90");
    ui->lockOrientationBox->addItem("180");
    ui->lockOrientationBox->addItem("270");
    ui->lockOrientationBox->setCurrentIndex(0);

    ui->recordPathEdt->setText(Config::getInstance().getRecordPath());
    ui->framelessCheck->setChecked(Config::getInstance().getFramelessWindow());

    on_useSingleModeCheck_clicked();

    updateConnectedList();

#ifdef Q_OS_OSX
    // mac need more width
    setFixedWidth(550);
#endif

#ifdef Q_OS_LINUX
    // linux need more width
    setFixedWidth(520);
#endif
}

void Dialog::execAdbCmd()
{
    if (checkAdbRun()) {
        return;
    }
    QString cmd = ui->adbCommandEdt->text().trimmed();
    outLog("adb " + cmd, false);
    m_adb.execute(ui->serialBox->currentText().trimmed(), cmd.split(" ", Qt::SkipEmptyParts));
}

void Dialog::delayMs(int ms)
{
    QTime dieTime = QTime::currentTime().addMSecs(ms);

    while (QTime::currentTime() < dieTime) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

QString Dialog::getGameScript(const QString &fileName)
{
    QFile loadFile(KeyMap::getKeyMapPath() + "/" + fileName);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        outLog("open file failed:" + fileName, true);
        return "";
    }

    QString ret = loadFile.readAll();
    loadFile.close();
    return ret;
}

void Dialog::slotShow()
{
    this->show();
    m_hideIcon->hide();
}

void Dialog::slotActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        this->show();
        m_hideIcon->hide();
        break;
    default:
        break;
    }
}

void Dialog::updateConnectedList()
{
    ui->connectedPhoneList->clear();
    QStringList list = Config::getInstance().getConnectedGroups();

    QRegExp regIP("\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\:([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])\\b");

    for (int i = 0; i < list.length(); ++i)
    {
        QString phone = QString(list[i]);
        if(phone != "common" /*&& regIP.exactMatch(phone)*/)
        {
            ui->connectedPhoneList->addItem(phone+"-"+Config::getInstance().getUserName(phone));
        }
    }
}

void Dialog::updateUser()
{

}

void Dialog::loadUser()
{

}

void Dialog::closeEvent(QCloseEvent *event)
{
    int res = QMessageBox::question(this,tr("warning"),tr("Quit or set tray?"),tr("Quit"),tr("Set tray"),tr("Cancel"));

    if(res == 0)
    {
       event->accept();
    }
    else if(res == 1)
    {
        this->hide();
        m_hideIcon->show();
        m_hideIcon->showMessage(tr("Notice"),
                              tr("Hidden here!"),
                              QSystemTrayIcon::Information,
                              3000);
        event->ignore();
    }
    else
    {
        event->ignore();
    }


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

    UserBootConfig config;

    config.recordScreen = ui->recordScreenCheck->isChecked();
    config.recordBackground = ui->notDisplayCheck->isChecked();
    config.reverseConnect = ui->useReverseCheck->isChecked();
    config.showFPS = ui->fpsCheck->isChecked();
    config.windowOnTop = ui->alwaysTopCheck->isChecked();
    config.autoOffScreen = ui->closeScreenCheck->isChecked();
    config.windowFrameless = ui->framelessCheck->isChecked();
    config.keepAlive = ui->stayAwakeCheck->isChecked();

    Config::getInstance().setUserBootConfig(ui->serialBox->currentText(),config);

    updateConnectedList();

    QString absFilePath;
    if (ui->recordScreenCheck->isChecked()) {
        QString fileDir(ui->recordPathEdt->text().trimmed());
        if (!fileDir.isEmpty()) {
            QDateTime dateTime = QDateTime::currentDateTime();
            QString fileName = dateTime.toString("_yyyyMMdd_hhmmss_zzz");
            QString ext = ui->formatBox->currentText().trimmed();
            fileName = windowTitle() + fileName + "." + ext;
            QDir dir(fileDir);
            absFilePath = dir.absoluteFilePath(fileName);
        }
    }

    quint32 bitRate = ui->bitRateBox->currentText().trimmed().toUInt();
    // this is ok that "native" toUshort is 0
    quint16 videoSize = ui->maxSizeBox->currentText().trimmed().toUShort();
    Device::DeviceParams params;
    params.serial = ui->serialBox->currentText().trimmed();
    params.maxSize = videoSize;
    params.bitRate = bitRate;
    // on devices with Android >= 10, the capture frame rate can be limited
    params.maxFps = static_cast<quint32>(Config::getInstance().getMaxFps());
    params.recordFileName = absFilePath;
    params.closeScreen = ui->closeScreenCheck->isChecked();
    params.useReverse = ui->useReverseCheck->isChecked();
    params.display = !ui->notDisplayCheck->isChecked();
    params.renderExpiredFrames = Config::getInstance().getRenderExpiredFrames();
    params.lockVideoOrientation = ui->lockOrientationBox->currentIndex() - 1;
    params.stayAwake = ui->stayAwakeCheck->isChecked();

    m_deviceManage.connectDevice(params);

    if (ui->alwaysTopCheck->isChecked()) {
        m_deviceManage.staysOnTop(params.serial);
    }
    m_deviceManage.showFPS(params.serial, ui->fpsCheck->isChecked());
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
    QTimer::singleShot(0, this, [this, backLog, newLine]() {
        ui->outEdit->append(backLog);
        if (newLine) {
            ui->outEdit->append("<br/>");
        }
    });
}

bool Dialog::filterLog(const QString &log)
{
    if (log.contains("app_proces")) {
        return true;
    }
    if (log.contains("Unable to set geometry")) {
        return true;
    }
    return false;
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

void Dialog::getIPbyIp()
{
    if (checkAdbRun()) {
        return;
    }

    QStringList adbArgs;
    adbArgs << "shell";
    adbArgs << "ip -o a";

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
    QString directory = QFileDialog::getExistingDirectory(this, tr("select path"), "", options);
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

void Dialog::on_recordScreenCheck_clicked(bool checked)
{
    if (!checked) {
        return;
    }

    QString fileDir(ui->recordPathEdt->text().trimmed());
    if (fileDir.isEmpty()) {
        qWarning() << "please select record save path!!!";
        ui->recordScreenCheck->setChecked(false);
    }
}

void Dialog::on_bitRateBox_activated(int index)
{
    Config::getInstance().setBitRateIndex(index);
}

void Dialog::on_maxSizeBox_activated(int index)
{
    Config::getInstance().setMaxSizeIndex(index);
}

void Dialog::on_formatBox_activated(int index)
{
    Config::getInstance().setRecordFormatIndex(index);
}

void Dialog::on_framelessCheck_stateChanged(int arg1)
{
    Q_UNUSED(arg1)
    Config::getInstance().setFramelessWindow(ui->framelessCheck->isChecked());
}

void Dialog::on_usbConnectBtn_clicked()
{
    on_stopAllServerBtn_clicked();
    delayMs(200);
    on_updateDevice_clicked();
    delayMs(200);
    if(ui->serialBox->count()==0)
    {
        qWarning() << "No device is found!";
        return;
    }

    QRegExp regIP("\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\:([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])\\b");

    for (int i = 0; i < ui->serialBox->count(); ++i)
    {
        if(!regIP.exactMatch(ui->serialBox->itemText(i)))
        {
            ui->serialBox->setCurrentIndex(i);
            on_startServerBtn_clicked();
            break;
        }
    }

    updateConnectedList();
}

void Dialog::on_wifiConnectBtn_clicked()
{
    on_stopAllServerBtn_clicked();
    delayMs(200);

    on_updateDevice_clicked();
    delayMs(200);
    if(ui->serialBox->count()==0)
    {
        qWarning() << "No device is found!";
        return;
    }

    on_getIPBtn_clicked();
    delayMs(200);

    on_startAdbdBtn_clicked();
    delayMs(1000);

    on_wirelessConnectBtn_clicked();
    delayMs(2000);

    ui->serialBox->clear();

    ui->serialBox->addItem(ui->deviceIpEdt->text()+":5555");

    on_startServerBtn_clicked();
    delayMs(200);

    updateConnectedList();
}

void Dialog::on_connectedPhoneList_itemDoubleClicked(QListWidgetItem *item)
{
    ui->serialBox->clear();
    ui->serialBox->addItem(item->text().split("-")[0]);
    ui->serialBox->setCurrentIndex(0);

    UserBootConfig config = Config::getInstance().getUserBootConfig(ui->serialBox->currentText());

    ui->recordScreenCheck->setChecked(config.recordScreen);
    ui->notDisplayCheck->setChecked(config.recordBackground);
    ui->useReverseCheck->setChecked(config.reverseConnect);
    ui->fpsCheck->setChecked(config.showFPS);
    ui->alwaysTopCheck->setChecked(config.windowOnTop);
    ui->closeScreenCheck->setChecked(config.autoOffScreen);
    ui->framelessCheck->setChecked(config.windowFrameless);
    ui->stayAwakeCheck->setChecked(config.keepAlive);
    ui->userNameEdt->setText(Config::getInstance().getUserName(ui->serialBox->currentText()));

    on_startServerBtn_clicked();
}

void Dialog::on_updateNameBtn_clicked()
{
    if(ui->serialBox->count()!=0)
    {
        if(ui->userNameEdt->text().isEmpty())
            Config::getInstance().setUserName(ui->serialBox->currentText(),"PHONE");
        else
            Config::getInstance().setUserName(ui->serialBox->currentText(),ui->userNameEdt->text());

        updateConnectedList();

        qDebug()<<"Update OK!";
    }
    else
    {
        qWarning()<<"No device is connected!";
    }
}

void Dialog::on_useSingleModeCheck_clicked()
{
    if(ui->useSingleModeCheck->isChecked())
    {
        ui->configGroupBox->hide();
        ui->adbGroupBox->hide();
        ui->wirelessGroupBox->hide();
        ui->usbGroupBox->hide();
    }
    else
    {
        ui->configGroupBox->show();
        ui->adbGroupBox->show();
        ui->wirelessGroupBox->show();
        ui->usbGroupBox->show();
    }
}
