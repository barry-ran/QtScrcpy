#include <QDebug>
#include <QHideEvent>
#include <QMouseEvent>
#include <QShowEvent>

#include "iconhelper.h"
#include "toolform.h"
#include "ui_toolform.h"
#include "videoform.h"
#include "../groupcontroller/groupcontroller.h"

ToolForm::ToolForm(QWidget *adsorbWidget, AdsorbPositions adsorbPos) : MagneticWidget(adsorbWidget, adsorbPos), ui(new Ui::ToolForm)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    //setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);

    updateGroupControl();

    initStyle();
}

ToolForm::~ToolForm()
{
    delete ui;
}

void ToolForm::setSerial(const QString &serial)
{
    m_serial = serial;
}

bool ToolForm::isHost()
{
    return m_isHost;
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
    IconHelper::Instance()->SetIcon(ui->openScreenBtn, QChar(0xf06e), 15);
    IconHelper::Instance()->SetIcon(ui->closeScreenBtn, QChar(0xf070), 15);
    IconHelper::Instance()->SetIcon(ui->powerBtn, QChar(0xf011), 15);
    IconHelper::Instance()->SetIcon(ui->expandNotifyBtn, QChar(0xf103), 15);
    IconHelper::Instance()->SetIcon(ui->screenShotBtn, QChar(0xf0c4), 15);
    IconHelper::Instance()->SetIcon(ui->touchBtn, QChar(0xf111), 15);
    IconHelper::Instance()->SetIcon(ui->groupControlBtn, QChar(0xf0c0), 15);
    IconHelper::Instance()->SetIcon(ui->clipboardBtn, QChar(0xf0c5), 15);
}

void ToolForm::updateGroupControl()
{
    if (m_isHost) {
        ui->groupControlBtn->setStyleSheet("color: red");
    } else {
        ui->groupControlBtn->setStyleSheet("color: green");
    }

    GroupController::instance().updateDeviceState(m_serial);
}

void ToolForm::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
#else
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
#endif
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
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        move(event->globalPos() - m_dragPosition);
#else
        move(event->globalPosition().toPoint() - m_dragPosition);
#endif
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
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }

    dynamic_cast<VideoForm*>(parent())->switchFullScreen();
}

void ToolForm::on_returnBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->postGoBack();
}

void ToolForm::on_homeBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->postGoHome();
}

void ToolForm::on_menuBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->postGoMenu();
}

void ToolForm::on_appSwitchBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->postAppSwitch();
}

void ToolForm::on_powerBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->postPower();
}

void ToolForm::on_screenShotBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->screenshot();
}

void ToolForm::on_volumeUpBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->postVolumeUp();
}

void ToolForm::on_volumeDownBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->postVolumeDown();
}

void ToolForm::on_closeScreenBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->setDisplayPower(false);
}

void ToolForm::on_expandNotifyBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->expandNotificationPanel();
}

void ToolForm::on_touchBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }

    m_showTouch = !m_showTouch;
    device->showTouch(m_showTouch);
}

void ToolForm::on_groupControlBtn_clicked()
{
    m_isHost = !m_isHost;
    updateGroupControl();
}

void ToolForm::on_openScreenBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->setDisplayPower(true);
}

void ToolForm::on_clipboardBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    device->requestDeviceClipboard();
}