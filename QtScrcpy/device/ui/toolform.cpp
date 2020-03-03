#include <QMouseEvent>
#include <QDebug>
#include <QShowEvent>
#include <QHideEvent>

#include "toolform.h"
#include "ui_toolform.h"
#include "iconhelper.h"
#include "device.h"
#include "videoform.h"
#include "controller.h"
#include "adbprocess.h"

ToolForm::ToolForm(QWidget* adsorbWidget, AdsorbPositions adsorbPos)
    : MagneticWidget(adsorbWidget, adsorbPos)
    , ui(new Ui::ToolForm)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    //setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);

    initStyle();
}

ToolForm::~ToolForm()
{
    delete ui;
}

void ToolForm::setDevice(Device *device)
{
    m_device = device;
}

void ToolForm::initStyle()
{
    IconHelper::Instance()->SetIcon(ui->fullScreenBtn, QChar(0xf0b2), 15);
    IconHelper::Instance()->SetIcon(ui->menuBtn, QChar(0xf096), 15);
    IconHelper::Instance()->SetIcon(ui->homeBtn, QChar(0xf1db), 15);
    //IconHelper::Instance()->SetIcon(ui->returnBtn, QChar(0xf104), 15);
    IconHelper::Instance()->SetIcon(ui->returnBtn, QChar(0xf053), 15);
    IconHelper::Instance()->SetIcon(ui->appSwitchBtn, QChar(0xf24d), 15);    
    IconHelper::Instance()->SetIcon(ui->volumeUpBtn, QChar(0xf028), 15);
    IconHelper::Instance()->SetIcon(ui->volumeDownBtn, QChar(0xf027), 15);
    IconHelper::Instance()->SetIcon(ui->closeScreenBtn, QChar(0xf070), 15);
    IconHelper::Instance()->SetIcon(ui->powerBtn, QChar(0xf011), 15);
    IconHelper::Instance()->SetIcon(ui->expandNotifyBtn, QChar(0xf103), 15);
    IconHelper::Instance()->SetIcon(ui->screenShotBtn, QChar(0xf0c4), 15);
    IconHelper::Instance()->SetIcon(ui->touchBtn, QChar(0xf111), 15);
    IconHelper::Instance()->SetIcon(ui->groupControlBtn, QChar(0xf0c0), 15);
}

void ToolForm::updateGroupControl()
{
    if (!m_device || !m_device->getVideoForm()) {
        return;
    }
    if (m_device->getVideoForm()->mainControl()) {
        ui->groupControlBtn->setStyleSheet("color: red");
    } else {
        ui->groupControlBtn->setStyleSheet("color: #DCDCDC");
    }
}

void ToolForm::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void ToolForm::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

void ToolForm::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void ToolForm::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    qDebug() << "show event";
}

void ToolForm::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    qDebug() << "hide event";
}

void ToolForm::on_fullScreenBtn_clicked()
{
    if (!m_device || !m_device->getVideoForm()) {
        return;
    }
    m_device->getVideoForm()->switchFullScreen();
}

void ToolForm::on_returnBtn_clicked()
{
    if (!m_device || !m_device->getController()) {
        return;
    }
    m_device->getController()->postGoBack();
}

void ToolForm::on_homeBtn_clicked()
{
    if (!m_device || !m_device->getController()) {
        return;
    }
    m_device->getController()->postGoHome();
}

void ToolForm::on_menuBtn_clicked()
{
    if (!m_device || !m_device->getController()) {
        return;
    }
    m_device->getController()->postGoMenu();
}

void ToolForm::on_appSwitchBtn_clicked()
{
    if (!m_device || !m_device->getController()) {
        return;
    }
    m_device->getController()->postAppSwitch();
}

void ToolForm::on_powerBtn_clicked()
{
    if (!m_device || !m_device->getController()) {
        return;
    }
    m_device->getController()->postPower();
}

void ToolForm::on_screenShotBtn_clicked()
{
    emit screenshot();
}

void ToolForm::on_volumeUpBtn_clicked()
{
    if (!m_device || !m_device->getController()) {
        return;
    }
    m_device->getController()->postVolumeUp();
}

void ToolForm::on_volumeDownBtn_clicked()
{
    if (!m_device || !m_device->getController()) {
        return;
    }
    m_device->getController()->postVolumeDown();
}

void ToolForm::on_closeScreenBtn_clicked()
{
    if (!m_device || !m_device->getController()) {
        return;
    }
     m_device->getController()->setScreenPowerMode(ControlMsg::SPM_OFF);
}

void ToolForm::on_expandNotifyBtn_clicked()
{
    if (!m_device || !m_device->getController()) {
        return;
    }
    m_device->getController()->expandNotificationPanel();
}

void ToolForm::on_touchBtn_clicked()
{
    if (!m_device) {
        return;
    }

    m_showTouch = !m_showTouch;

    AdbProcess* adb = new AdbProcess();
    if (!adb) {
        return;
    }
    connect(adb, &AdbProcess::adbProcessResult, this, [this](AdbProcess::ADB_EXEC_RESULT processResult){
        if (AdbProcess::AER_SUCCESS_START != processResult) {
            sender()->deleteLater();
        }
    });
    adb->setShowTouchesEnabled(m_device->getSerial(), m_showTouch);

    qInfo() << "show touch " << (m_showTouch ? "enable" : "disable");
}

void ToolForm::on_groupControlBtn_clicked()
{
    if (!m_device || !m_device->getVideoForm()) {
        return;
    }
    m_device->getVideoForm()->setMainControl(!m_device->getVideoForm()->mainControl());
    updateGroupControl();
}
