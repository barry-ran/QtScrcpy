#include <QDebug>
#include <QHideEvent>
#include <QMouseEvent>
#include <QShowEvent>

#include "device.h"
#include "iconhelper.h"
#include "toolform.h"
#include "ui_toolform.h"

ToolForm::ToolForm(QWidget *adsorbWidget, AdsorbPositions adsorbPos) : MagneticWidget(adsorbWidget, adsorbPos), ui(new Ui::ToolForm)
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
    if (!device) {
        return;
    }
    m_device = device;
    connect(m_device, &Device::controlStateChange, this, &ToolForm::onControlStateChange);
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
    if (!m_device) {
        return;
    }
    switch (m_device->controlState()) {
    case Device::GroupControlState::GCS_FREE:
        ui->groupControlBtn->setStyleSheet("color: #DCDCDC");
        break;
    case Device::GroupControlState::GCS_HOST:
        ui->groupControlBtn->setStyleSheet("color: red");
        break;
    case Device::GroupControlState::GCS_CLIENT:
        ui->groupControlBtn->setStyleSheet("color: green");
        break;
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
    if (!m_device) {
        return;
    }

    emit m_device->switchFullScreen();
}

void ToolForm::on_returnBtn_clicked()
{
    if (!m_device) {
        return;
    }
    emit m_device->postGoBack();
}

void ToolForm::on_homeBtn_clicked()
{
    if (!m_device) {
        return;
    }
    emit m_device->postGoHome();
}

void ToolForm::on_menuBtn_clicked()
{
    if (!m_device) {
        return;
    }
    emit m_device->postGoMenu();
}

void ToolForm::on_appSwitchBtn_clicked()
{
    if (!m_device) {
        return;
    }
    emit m_device->postAppSwitch();
}

void ToolForm::on_powerBtn_clicked()
{
    if (!m_device) {
        return;
    }
    emit m_device->postPower();
}

void ToolForm::on_screenShotBtn_clicked()
{
    if (!m_device) {
        return;
    }
    emit m_device->screenshot();
}

void ToolForm::on_volumeUpBtn_clicked()
{
    if (!m_device) {
        return;
    }
    emit m_device->postVolumeUp();
}

void ToolForm::on_volumeDownBtn_clicked()
{
    if (!m_device) {
        return;
    }
    emit m_device->postVolumeDown();
}

void ToolForm::on_closeScreenBtn_clicked()
{
    if (!m_device) {
        return;
    }
    emit m_device->setScreenPowerMode(ControlMsg::SPM_OFF);
}

void ToolForm::on_expandNotifyBtn_clicked()
{
    if (!m_device) {
        return;
    }
    emit m_device->expandNotificationPanel();
}

void ToolForm::on_touchBtn_clicked()
{
    if (!m_device) {
        return;
    }

    m_showTouch = !m_showTouch;
    emit m_device->showTouch(m_showTouch);
}

void ToolForm::on_groupControlBtn_clicked()
{
    if (!m_device) {
        return;
    }
    Device::GroupControlState state = m_device->controlState();
    if (state == Device::GroupControlState::GCS_FREE) {
        emit m_device->setControlState(m_device, Device::GroupControlState::GCS_HOST);
    }
    if (state == Device::GroupControlState::GCS_HOST) {
        emit m_device->setControlState(m_device, Device::GroupControlState::GCS_FREE);
    }
}

void ToolForm::onControlStateChange(Device *device, Device::GroupControlState oldState, Device::GroupControlState newState)
{
    Q_UNUSED(device)
    Q_UNUSED(oldState)
    Q_UNUSED(newState)
    updateGroupControl();
}
