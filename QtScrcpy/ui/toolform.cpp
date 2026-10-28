#include "videoform.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QDebug>
#include <QHideEvent>
#include <QMouseEvent>
#include <QShowEvent>

#include <QIcon>
#include <QMenu>
#include <QScreen>
#include <QWindow>
#include <QGuiApplication>
#include <QPainter>
#include "toolform.h"
#include "ui_toolform.h"
#include "videoform.h"
#include "../groupcontroller/groupcontroller.h"

ToolForm::ToolForm(QWidget *adsorbWidget, MagneticWidget::AdsorbPositions adsorbPos) : QWidget(adsorbWidget), ui(new Ui::ToolForm)
{
    Q_UNUSED(adsorbPos)
    ui->setupUi(this);
    setAttribute(Qt::WA_ShowWithoutActivating);

    delete layout();
    m_compactGrid = new QGridLayout(this);
    m_compactGrid->setContentsMargins(6,6,6,6);
    m_compactGrid->setSpacing(3);
    m_audioButton=new QPushButton(this);m_audioButton->setObjectName("ProjectionAudioToggle");m_audioButton->setCheckable(true);
    setAudioState(false,false);
    connect(m_audioButton,&QPushButton::clicked,this,&ToolForm::audioToggleRequested);
    m_compactButtons = {m_audioButton,ui->homeBtn,ui->returnBtn,ui->appSwitchBtn,ui->expandSettingsBtn,ui->expandNotifyBtn};
    m_extraButtons = {ui->fullScreenBtn,ui->rotateBtn,ui->screenShotBtn,ui->volumeUpBtn,ui->volumeDownBtn,ui->powerBtn,ui->openScreenBtn, ui->closeScreenBtn,
        ui->touchBtn, ui->clipboardBtn,
        ui->groupControlBtn, ui->menuBtn, ui->cameraTorchBtn, ui->cameraZoomOutBtn, ui->cameraZoomInBtn};
    for (auto *button : m_extraButtons) button->hide();
    auto *more = new QPushButton(this);
    more->setAccessibleName(QStringLiteral("更多工具"));
    more->setToolTip(QStringLiteral("更多工具"));
    QPixmap dots(24,24); dots.fill(Qt::transparent);
    { QPainter p(&dots); p.setRenderHint(QPainter::Antialiasing); p.setPen(Qt::NoPen); p.setBrush(QColor("#526450"));
      for (int x : {5,12,19}) p.drawEllipse(QPointF(x,12),1.6,1.6); }
    more->setIcon(QIcon(dots));
    m_compactButtons.append(more);
    connect(more, &QPushButton::clicked, this, [this, more]() {
        // Parent native popups to the host, never a non-native chrome child.
        auto *menu = new QMenu(parentWidget());
        menu->setAttribute(Qt::WA_DeleteOnClose);
        for (auto *button : m_extraButtons) {
            if(m_fullscreen&&button==ui->fullScreenBtn)continue;
            auto *action = menu->addAction(button->icon(), button->accessibleName());
            action->setEnabled(button->isEnabled());
            connect(action, &QAction::triggered, button, &QPushButton::click);
        }
        menu->ensurePolished();
        const QSize menuSize=menu->sizeHint();
        const auto *form=qobject_cast<VideoForm*>(parentWidget());
        const QRect phone=form?QRect(form->mapToGlobal(form->phoneRect().topLeft()),form->phoneRect().size()):parentWidget()->frameGeometry();
        QScreen *screen=QGuiApplication::screenAt(phone.center());
        const QRect available=screen?screen->availableGeometry():phone;
        // Anchor to the toolbar itself, on the same side of the projection.
        const QRect strip(mapToGlobal(QPoint()),size());
        int x=strip.center().x()<phone.center().x()?strip.left()-menuSize.width():strip.right()+1;
        int y=more->mapToGlobal(QPoint()).y();
        if(m_horizontal){x=more->mapToGlobal(QPoint()).x();y=strip.bottom()+1;}
        if(x<available.left())x=strip.right()+1;
        if(x+menuSize.width()>available.right()+1)x=strip.left()-menuSize.width();
        x=qBound(available.left(),x,qMax(available.left(),available.right()-menuSize.width()+1));
        y=qBound(available.top(),y,qMax(available.top(),available.bottom()-menuSize.height()+1));
        menu->popup(QPoint(x,y));
    });
    for (auto *button : m_compactButtons) button->setFixedSize(34,34);
    setHorizontal(false);


    //setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);

    updateGroupControl();

    initStyle();
    ui->expandNotifyBtn->setCheckable(true);
    ui->expandSettingsBtn->setCheckable(true);
    resetPanel();
}

void ToolForm::setHorizontal(bool horizontal)
{
    if(m_layoutReady&&m_horizontal==horizontal)return;
    m_layoutReady=true;
    m_horizontal = horizontal;
    for (int i=0; i<m_compactButtons.size(); ++i) {
        auto *button=m_compactButtons[i]; m_compactGrid->removeWidget(button);
        m_compactGrid->addWidget(button, horizontal?0:i, horizontal?i:0); button->show();
    }
    const int length=12 + m_compactButtons.size()*34 + (m_compactButtons.size()-1)*3;
    setFixedSize(horizontal?length:46, horizontal?46:length);
}

void ToolForm::setFullscreenMode(bool fullscreen)
{
    if(m_fullscreen==fullscreen)return;
    m_fullscreen=fullscreen;
    if(fullscreen)m_compactButtons.insert(m_compactButtons.size()-1,ui->fullScreenBtn);
    else {m_compactButtons.removeAll(ui->fullScreenBtn);m_compactGrid->removeWidget(ui->fullScreenBtn);ui->fullScreenBtn->hide();}
    m_layoutReady=false;setHorizontal(m_horizontal);
    setAttribute(Qt::WA_StyledBackground,true);
    setStyleSheet(fullscreen?QStringLiteral("ToolForm{background:rgba(28,31,29,225);border:1px solid #515952;border-radius:12px;} QPushButton{background:transparent;border:0;border-radius:7px;padding:0;} QPushButton:hover{background:#495149;} QPushButton:pressed,QPushButton:checked{background:#546458;}"):QStringLiteral("ToolForm{background:transparent;border:0;} QPushButton{background:#fcfcfa;border:0;border-radius:7px;padding:0;} QPushButton:hover{background:#e7ebe2;} QPushButton:checked{background:#dce7d9;}"));
    initStyle();
    ui->fullScreenBtn->setAccessibleName(fullscreen?QStringLiteral("退出全屏"):QStringLiteral("全屏"));
    ui->fullScreenBtn->setToolTip(fullscreen?QStringLiteral("退出全屏（Esc）"):QStringLiteral("全屏"));
    QPixmap dots(24,24);dots.fill(Qt::transparent);
    {QPainter p(&dots);p.setRenderHint(QPainter::Antialiasing);p.setPen(Qt::NoPen);p.setBrush(QColor("#526450"));for(int x:{5,12,19})p.drawEllipse(QPointF(x,12),1.6,1.6);}
    m_compactButtons.last()->setIcon(QIcon(dots));
    for(auto *button:m_compactButtons) {
        button->setFixedSize(34,34);
        if(button==m_audioButton)continue;
        QPixmap pix=button->icon().pixmap(QSize(40,40));
        if(fullscreen){QPainter p(&pix);p.setCompositionMode(QPainter::CompositionMode_SourceIn);p.fillRect(pix.rect(),QColor("#edf1eb"));}
        button->setIcon(QIcon(pix));
    }
    setAudioState(m_audioActive,m_audioStarting);
}

void ToolForm::setAudioState(bool active,bool starting)
{
    m_audioActive=active;m_audioStarting=starting;
    m_audioButton->setChecked(active||starting);
    const QString action=starting?QStringLiteral("取消音频启动"):(active?QStringLiteral("关闭音频"):QStringLiteral("打开音频"));
    m_audioButton->setAccessibleName(action);m_audioButton->setToolTip(action);
    QPixmap symbol(48,48);symbol.setDevicePixelRatio(2);symbol.fill(Qt::transparent);
    QPainter p(&symbol);p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(m_fullscreen?"#edf1eb":(active||starting?"#55745c":"#526450")),1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);p.drawArc(QRectF(4,3,16,16),0,180*16);
    p.drawRoundedRect(QRectF(3,10,4,9),1.5,1.5);p.drawRoundedRect(QRectF(17,10,4,9),1.5,1.5);
    if(active||starting){p.setPen(Qt::NoPen);p.setBrush(QColor("#6b886f"));p.drawEllipse(QPointF(12,19),2.2,2.2);}
    m_audioButton->setIcon(QIcon(symbol));m_audioButton->setIconSize(QSize(22,22));
}

ToolForm::~ToolForm()
{
    delete ui;
}

void ToolForm::setSerial(const QString &serial)
{
    resetPanel();
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

    for (auto *button : m_extraButtons) button->hide();
    ui->rotateBtn->setEnabled(!camera);
    ui->volumeUpBtn->setEnabled(!camera); ui->volumeDownBtn->setEnabled(!camera);
    ui->homeBtn->setVisible(!camera); ui->returnBtn->setVisible(!camera);
    for (auto *button : {ui->cameraTorchBtn, ui->cameraZoomOutBtn, ui->cameraZoomInBtn}) button->setEnabled(camera);

}

void ToolForm::initStyle()
{
    // Real SVG icons remain independent of the application's typography.
    auto icon = [](QPushButton *button, const QString &resource, const QString &label) {
        button->setText(QString());
        button->setIcon(QIcon(resource));
        button->setIconSize(QSize(20,20));
        button->setAccessibleName(label);
    };
    icon(ui->fullScreenBtn, QStringLiteral(":/image/tools/fullScreenBtn.svg"), QStringLiteral("全屏"));
    icon(ui->menuBtn, QStringLiteral(":/image/tools/menuBtn.svg"), QStringLiteral("菜单"));
    icon(ui->homeBtn, QStringLiteral(":/image/tools/homeBtn.svg"), QStringLiteral("主页"));
    icon(ui->returnBtn, QStringLiteral(":/image/tools/returnBtn.svg"), QStringLiteral("返回"));
    icon(ui->appSwitchBtn, QStringLiteral(":/image/tools/appSwitchBtn.svg"), QStringLiteral("最近任务"));
    icon(ui->volumeUpBtn, QStringLiteral(":/image/tools/volumeUpBtn.svg"), QStringLiteral("增加音量"));
    icon(ui->volumeDownBtn, QStringLiteral(":/image/tools/volumeDownBtn.svg"), QStringLiteral("减小音量"));
    icon(ui->openScreenBtn, QStringLiteral(":/image/tools/openScreenBtn.svg"), QStringLiteral("打开屏幕"));
    icon(ui->closeScreenBtn, QStringLiteral(":/image/tools/closeScreenBtn.svg"), QStringLiteral("关闭屏幕"));
    icon(ui->powerBtn, QStringLiteral(":/image/tools/powerBtn.svg"), QStringLiteral("电源"));
    icon(ui->expandNotifyBtn, QStringLiteral(":/image/tools/expandNotifyBtn.svg"), QStringLiteral("通知栏"));
    icon(ui->expandSettingsBtn, QStringLiteral(":/image/tools/expandSettingsBtn.svg"), QStringLiteral("系统设置"));
    icon(ui->rotateBtn, QStringLiteral(":/image/tools/rotateBtn.svg"), QStringLiteral("旋转设备"));
    icon(ui->screenShotBtn, QStringLiteral(":/image/tools/screenShotBtn.svg"), QStringLiteral("截图"));
    icon(ui->touchBtn, QStringLiteral(":/image/tools/touchBtn.svg"), QStringLiteral("显示触摸"));
    icon(ui->groupControlBtn, QStringLiteral(":/image/tools/groupControlBtn.svg"), QStringLiteral("群控"));
    icon(ui->clipboardBtn, QStringLiteral(":/image/tools/clipboardBtn.svg"), QStringLiteral("复制手机剪贴板"));
    icon(ui->cameraTorchBtn, QStringLiteral(":/image/tools/cameraTorchBtn.svg"), QStringLiteral("相机补光灯"));
    icon(ui->cameraZoomOutBtn, QStringLiteral(":/image/tools/cameraZoomOutBtn.svg"), QStringLiteral("缩小"));
    icon(ui->cameraZoomInBtn, QStringLiteral(":/image/tools/cameraZoomInBtn.svg"), QStringLiteral("放大"));
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

void ToolForm::mousePressEvent(QMouseEvent *event){QWidget::mousePressEvent(event);}
void ToolForm::mouseReleaseEvent(QMouseEvent *event){QWidget::mouseReleaseEvent(event);}
void ToolForm::mouseMoveEvent(QMouseEvent *event){QWidget::mouseMoveEvent(event);}

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
    resetPanel();
    device->postGoBack();
}

void ToolForm::on_homeBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    resetPanel();
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
    resetPanel();
    device->postAppSwitch();
}

void ToolForm::on_powerBtn_clicked()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    resetPanel();
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

void ToolForm::resetPanel()
{
    m_openPanel = 0;
    ui->expandNotifyBtn->setChecked(false);
    ui->expandSettingsBtn->setChecked(false);
    ui->expandNotifyBtn->setToolTip(QStringLiteral("打开通知栏"));
    ui->expandSettingsBtn->setToolTip(QStringLiteral("打开快捷设置"));
}

void ToolForm::togglePanel(int panel)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) { resetPanel(); return; }
    if (m_openPanel == panel) {
        device->collapsePanel();
        resetPanel();
        return;
    }
    // Collapse before switching so OEM quick settings cannot remain layered over notifications.
    if (m_openPanel) device->collapsePanel();
    if (panel == 1) device->expandNotificationPanel();
    else device->expandSettingsPanel();
    m_openPanel = panel;
    ui->expandNotifyBtn->setChecked(panel == 1);
    ui->expandSettingsBtn->setChecked(panel == 2);
    ui->expandNotifyBtn->setToolTip(panel == 1 ? QStringLiteral("收起通知栏") : QStringLiteral("打开通知栏"));
    ui->expandSettingsBtn->setToolTip(panel == 2 ? QStringLiteral("收起快捷设置") : QStringLiteral("打开快捷设置"));
}

void ToolForm::on_expandNotifyBtn_clicked() { togglePanel(1); }
void ToolForm::on_expandSettingsBtn_clicked() { togglePanel(2); }

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