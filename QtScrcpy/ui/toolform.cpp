#include <QDebug>
#include <QHideEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidgetAction>

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
    initPrivacyControls();
}

ToolForm::~ToolForm()
{
    delete ui;
}

void ToolForm::setSerial(const QString &serial)
{
    m_serial = serial;
    updateCameraMode();
}

bool ToolForm::isHost()
{
    return m_isHost;
}

void ToolForm::updateCameraMode()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    const bool camera = device && device->isCameraMode();

    ui->groupControlBtn->setVisible(!camera);
    ui->expandNotifyBtn->setVisible(!camera);
    ui->expandSettingsBtn->setVisible(!camera);
    ui->rotateBtn->setVisible(!camera);
    ui->touchBtn->setVisible(!camera);
    ui->openScreenBtn->setVisible(!camera);
    ui->closeScreenBtn->setVisible(!camera);
    ui->powerBtn->setVisible(!camera);
    ui->volumeUpBtn->setVisible(!camera);
    ui->volumeDownBtn->setVisible(!camera);
    ui->appSwitchBtn->setVisible(!camera);
    ui->menuBtn->setVisible(!camera);
    ui->homeBtn->setVisible(!camera);
    ui->returnBtn->setVisible(!camera);
    ui->clipboardBtn->setVisible(!camera);
    ui->cameraTorchBtn->setVisible(camera);
    ui->cameraZoomOutBtn->setVisible(camera);
    ui->cameraZoomInBtn->setVisible(camera);
}

void ToolForm::initStyle()
{
    IconHelper::Instance()->SetIcon(ui->opacityBtn, QChar(0xf042), 15);
    IconHelper::Instance()->SetIcon(ui->autoHideBtn, QChar(0xf21b), 15);
    ui->autoHideBtn->setStyleSheet("QPushButton:checked { color: #55aaff; background: #173247; border-color: #0078d4; }");
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
    IconHelper::Instance()->SetIcon(ui->expandSettingsBtn, QChar(0xf013), 15);
    IconHelper::Instance()->SetIcon(ui->rotateBtn, QChar(0xf021), 15);
    IconHelper::Instance()->SetIcon(ui->screenShotBtn, QChar(0xf0c4), 15);
    IconHelper::Instance()->SetIcon(ui->touchBtn, QChar(0xf111), 15);
    IconHelper::Instance()->SetIcon(ui->groupControlBtn, QChar(0xf0c0), 15);
    IconHelper::Instance()->SetIcon(ui->clipboardBtn, QChar(0xf0c5), 15);
    IconHelper::Instance()->SetIcon(ui->cameraTorchBtn, QChar(0xf0eb), 15);
    IconHelper::Instance()->SetIcon(ui->cameraZoomOutBtn, QChar(0xf010), 15);
    IconHelper::Instance()->SetIcon(ui->cameraZoomInBtn, QChar(0xf00e), 15);
}

void ToolForm::initPrivacyControls()
{
    m_videoForm = qobject_cast<VideoForm *>(parent());
    ui->opacityBtn->setEnabled(m_videoForm);
    ui->autoHideBtn->setEnabled(m_videoForm);
    if (!m_videoForm) {
        return;
    }

    // Keep the popup owned by this toolbar so it is part of the auto-hide
    // interaction area, including while the pointer is over the slider.
    m_opacityMenu = new QMenu(this);
    m_opacityMenu->setObjectName("opacityMenu");
    auto action = new QWidgetAction(m_opacityMenu);
    auto content = new QWidget(m_opacityMenu);
    content->setMinimumWidth(240);
    auto layout = new QVBoxLayout(content);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(10);

    m_opacityLabel = new QLabel(content);
    m_opacityLabel->setObjectName("opacityValueLabel");
    layout->addWidget(m_opacityLabel);

    m_opacitySlider = new QSlider(Qt::Horizontal, content);
    m_opacitySlider->setObjectName("opacitySlider");
    m_opacitySlider->setRange(10, 100);
    m_opacitySlider->setSingleStep(1);
    m_opacitySlider->setPageStep(10);
    m_opacitySlider->setTracking(true);
    m_opacitySlider->setAccessibleName(tr("窗口不透明度"));
    m_opacityLabel->setBuddy(m_opacitySlider);
    layout->addWidget(m_opacitySlider);

    auto hint = new QLabel(tr("10% 更透明，100% 完全不透明"), content);
    layout->addWidget(hint);
    action->setDefaultWidget(content);
    m_opacityMenu->addAction(action);

    updateOpacityControls(m_videoForm->opacityPercent());
    connect(m_opacitySlider, &QSlider::valueChanged, m_videoForm, &VideoForm::setOpacityPercent);
    connect(m_videoForm, &VideoForm::opacityPercentChanged, this, &ToolForm::updateOpacityControls);
    connect(m_videoForm, &VideoForm::autoHideEnabledChanged, this, [this](bool enabled) {
        const QSignalBlocker blocker(ui->autoHideBtn);
        ui->autoHideBtn->setChecked(enabled);
    });
    const QSignalBlocker blocker(ui->autoHideBtn);
    ui->autoHideBtn->setChecked(m_videoForm->autoHideEnabled());
}

void ToolForm::updateOpacityControls(int percent)
{
    if (!m_opacitySlider || !m_opacityLabel) {
        return;
    }
    const QSignalBlocker blocker(m_opacitySlider);
    m_opacitySlider->setValue(percent);
    m_opacityLabel->setText(tr("窗口不透明度：%1%").arg(percent));
    ui->opacityBtn->setToolTip(tr("手动调节透明度（当前不透明度：%1%）\n同时调整投屏窗口与右侧工具栏").arg(percent));
}

void ToolForm::on_opacityBtn_clicked()
{
    if (!m_opacityMenu || !m_videoForm) {
        return;
    }
    updateOpacityControls(m_videoForm->opacityPercent());
    m_opacityMenu->popup(ui->opacityBtn->mapToGlobal(QPoint(-m_opacityMenu->sizeHint().width(), 0)));
    m_opacitySlider->setFocus(Qt::PopupFocusReason);
}

void ToolForm::on_autoHideBtn_toggled(bool enabled)
{
    if (m_videoForm) {
        m_videoForm->setAutoHideEnabled(enabled);
    }
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

void ToolForm::on_expandSettingsBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (device) {
        device->expandSettingsPanel();
    }
}

void ToolForm::on_rotateBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (device) {
        device->rotateDevice();
    }
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

void ToolForm::on_cameraTorchBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device || !device->isCameraMode()) {
        return;
    }
    m_cameraTorch = !m_cameraTorch;
    device->setCameraTorch(m_cameraTorch);
    ui->cameraTorchBtn->setStyleSheet(m_cameraTorch ? "color: #f0c419" : "");
}

void ToolForm::on_cameraZoomOutBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (device && device->isCameraMode()) {
        device->cameraZoomOut();
    }
}

void ToolForm::on_cameraZoomInBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (device && device->isCameraMode()) {
        device->cameraZoomIn();
    }
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
