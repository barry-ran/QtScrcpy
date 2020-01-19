#include <QMouseEvent>
#include <QDebug>
#include <QShowEvent>
#include <QHideEvent>

#include "toolform.h"
#include "ui_toolform.h"
#include "iconhelper.h"
#include "videoform.h"
#include "controller.h"

ToolForm::ToolForm(QWidget* adsorbWidget, AdsorbPositions adsorbPos)
    : MagneticWidget(adsorbWidget, adsorbPos)
    , ui(new Ui::ToolForm)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    //setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);

    m_videoForm = dynamic_cast<VideoForm*>(adsorbWidget);

    initStyle();
}

ToolForm::~ToolForm()
{
    delete ui;
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
    IconHelper::Instance()->SetIcon(ui->screenShotBtn, QChar(0xf05b), 15);
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
    Q_UNUSED(event);
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
    qDebug() << "show event";
}

void ToolForm::hideEvent(QHideEvent *event)
{
    qDebug() << "hide event";
}

void ToolForm::on_fullScreenBtn_clicked()
{
    if (m_videoForm) {
        m_videoForm->switchFullScreen();
    }
}

void ToolForm::on_returnBtn_clicked()
{
    if (m_videoForm && m_videoForm->getController()) {
        m_videoForm->getController()->postGoBack();
    }
}

void ToolForm::on_homeBtn_clicked()
{
    if (m_videoForm && m_videoForm->getController()) {
        m_videoForm->getController()->postGoHome();
    }
}

void ToolForm::on_menuBtn_clicked()
{
    if (m_videoForm && m_videoForm->getController()) {
        m_videoForm->getController()->postGoMenu();
    }
}

void ToolForm::on_appSwitchBtn_clicked()
{
    if (m_videoForm && m_videoForm->getController()) {
        m_videoForm->getController()->postAppSwitch();
    }
}

void ToolForm::on_powerBtn_clicked()
{
    if (m_videoForm && m_videoForm->getController()) {
        m_videoForm->getController()->postPower();
    }
}

void ToolForm::on_screenShotBtn_clicked()
{
    if (m_videoForm && m_videoForm->getController()) {
        m_videoForm->getController()->screenShot();
    }
}

void ToolForm::on_volumeUpBtn_clicked()
{
    if (m_videoForm && m_videoForm->getController()) {
        m_videoForm->getController()->postVolumeUp();
    }
}

void ToolForm::on_volumeDownBtn_clicked()
{
    if (m_videoForm && m_videoForm->getController()) {
        m_videoForm->getController()->postVolumeDown();
    }
}

void ToolForm::on_closeScreenBtn_clicked()
{
    if (m_videoForm && m_videoForm->getController()) {
        m_videoForm->getController()->setScreenPowerMode(ControlMsg::SPM_OFF);
    }
}

void ToolForm::on_expandNotifyBtn_clicked()
{
    if (m_videoForm && m_videoForm->getController()) {
        m_videoForm->getController()->expandNotificationPanel();
    }
}
