#include "dialog.h"
#include "ui_dialog.h"
#include "config.h"
#include <QAbstractItemModel>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>
namespace {
QLabel *label(const QString &s, const char *role="muted") { auto *l=new QLabel(s); l->setProperty("role",role);l->setWordWrap(true);return l; }
QWidget *row(std::initializer_list<QWidget*> widgets) {auto *w=new QWidget;auto *l=new QHBoxLayout(w);l->setContentsMargins(0,0,0,0);l->setSpacing(10);for(auto *c:widgets){l->addWidget(c);c->show();}return w;}
QFormLayout *section(QVBoxLayout *parent,const QString &title) {
    auto *heading=label(title,"section");parent->addWidget(heading);auto *f=new QFormLayout;
    f->setHorizontalSpacing(24);f->setVerticalSpacing(15);f->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);parent->addLayout(f);return f;
}
void field(QFormLayout *f,const QString &s,QWidget *w){f->addRow(s,w);w->show();}
void rule(QVBoxLayout *l){auto *r=new QFrame;r->setObjectName("Divider");r->setFixedHeight(1);l->addSpacing(7);l->addWidget(r);l->addSpacing(7);}
class DevicePortrait final:public QWidget {
public: explicit DevicePortrait(QWidget *p=nullptr):QWidget(p){setFixedSize(146,224);setAccessibleName(QStringLiteral("设备示意图，非实时投屏"));}
protected:void paintEvent(QPaintEvent*)override {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#b6b9b1"),1.5));p.setBrush(QColor("#e1e4dc"));p.drawRoundedRect(QRectF(13,3,120,216),19,19);
    p.setPen(Qt::NoPen);p.setBrush(QColor("#f5f5f0"));p.drawRoundedRect(QRectF(18,8,110,206),15,15);
    p.setBrush(QColor("#abb4a6"));p.drawEllipse(QPointF(73,17),2.3,2.3);
    p.drawPixmap(QRect(45,77,56,56),QPixmap(":/image/tray/logo.png"));
    p.setPen(QColor("#798473"));QFont f=font();f.setPixelSize(11);p.setFont(f);p.drawText(QRect(20,145,106,20),Qt::AlignCenter,QStringLiteral("独立窗口投屏"));
    p.setPen(QPen(QColor("#b5bcb0"),3,Qt::SolidLine,Qt::RoundCap));p.drawLine(57,205,89,205);
}}
;
}
void Dialog::initWorkspaceUi()
{
    for(auto *child:findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly))child->hide();delete layout();setObjectName("Workspace");
    auto *shell=new QVBoxLayout(this);shell->setContentsMargins(0,0,0,0);shell->setSpacing(0);
    auto *body=new QWidget;auto *split=new QHBoxLayout(body);split->setContentsMargins(0,0,0,0);split->setSpacing(0);shell->addWidget(body,1);
    auto *side=new QFrame;side->setObjectName("Sidebar");side->setFixedWidth(270);auto *sl=new QVBoxLayout(side);sl->setContentsMargins(26,25,26,22);sl->setSpacing(11);
    sl->addWidget(label(QStringLiteral("我的设备"),"section"));sl->addSpacing(10);sl->addWidget(new DevicePortrait,0,Qt::AlignHCenter);
    auto *name=label(QStringLiteral("等待连接"),"deviceTitle");name->setAlignment(Qt::AlignCenter);sl->addWidget(name);
    auto *identity=label(QStringLiteral("连接手机并允许 USB 调试"));identity->setAlignment(Qt::AlignCenter);sl->addWidget(identity);
    auto *status=label(QStringLiteral("未连接设备"),"connection");status->setAlignment(Qt::AlignCenter);sl->addWidget(status);
    ui->connectedPhoneList->setFixedHeight(73);ui->connectedPhoneList->setObjectName("DeviceList");sl->addWidget(ui->connectedPhoneList);ui->connectedPhoneList->show();
    // Keep the original serial selector for existing actions, without a redundant second list.
    ui->serialBox->setParent(side);ui->serialBox->hide();
    connect(ui->connectedPhoneList,&QListWidget::currentRowChanged,this,[this](int i){if(i>=0)ui->serialBox->setCurrentIndex(i);});
    auto update=[=](){if(m_shuttingDown)return;const QString serial=ui->serialBox->currentText();const bool available=!serial.isEmpty();
        QString nick=Config::getInstance().getNickName(serial);if(nick.isEmpty()||nick==QStringLiteral("Phone"))nick=QStringLiteral("Android 设备");
        ui->connectedPhoneList->setVisible(ui->connectedPhoneList->count()>1);
        name->setText(available?nick:QStringLiteral("等待连接"));identity->setText(available?serial:QStringLiteral("连接手机并允许 USB 调试"));
        status->setText(available?(serial.contains(':')?QStringLiteral("无线 · 已连接"):QStringLiteral("USB · 已连接")):QStringLiteral("未连接设备"));
        ui->startServerBtn->setEnabled(available);ui->stopServerBtn->setEnabled(available);ui->startAudioBtn->setEnabled(available);ui->stopAudioBtn->setEnabled(available);
    };
    connect(ui->serialBox,&QComboBox::currentTextChanged,this,[=](){update();});
    connect(ui->connectedPhoneList->model(),&QAbstractItemModel::modelReset,this,update);
    connect(ui->connectedPhoneList->model(),&QAbstractItemModel::rowsInserted,this,[=](){update();});
    connect(ui->connectedPhoneList->model(),&QAbstractItemModel::rowsRemoved,this,[=](){update();});
    sl->addStretch();ui->updateDevice->setText(QStringLiteral("刷新设备"));sl->addWidget(ui->updateDevice);ui->updateDevice->show();ui->autoUpdatecheckBox->setText(QStringLiteral("自动发现设备"));sl->addWidget(ui->autoUpdatecheckBox);ui->autoUpdatecheckBox->show();split->addWidget(side);
    auto *main=new QWidget;auto *ml=new QVBoxLayout(main);ml->setContentsMargins(30,23,30,22);ml->setSpacing(18);split->addWidget(main,1);
    auto *nav=new QHBoxLayout;nav->setSpacing(24);auto *group=new QButtonGroup(this);group->setExclusive(true);auto *pages=new QStackedWidget;QVector<QVBoxLayout*> layouts;
    const QStringList titles={QStringLiteral("投屏"),QStringLiteral("音频"),QStringLiteral("按键"),QStringLiteral("连接"),QStringLiteral("高级")};
    for(int i=0;i<titles.size();++i){auto *button=new QPushButton(titles[i]);button->setObjectName("LinenTab");button->setCheckable(true);group->addButton(button,i);nav->addWidget(button);
        auto *scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);auto *page=new QWidget;page->setObjectName("Page");auto *l=new QVBoxLayout(page);l->setContentsMargins(0,10,8,12);l->setSpacing(14);layouts.append(l);scroll->setWidget(page);if(i==4){scroll->takeWidget();pages->addWidget(page);delete scroll;}else{pages->addWidget(scroll);}
        connect(button,&QPushButton::toggled,this,[=](bool selected){if(selected)pages->setCurrentIndex(i);});if(i==0)button->setChecked(true);
    }nav->addStretch();ml->addLayout(nav);ml->addWidget(pages,1);
    auto *home=layouts[0];auto *quality=section(home,QStringLiteral("画面"));field(quality,QStringLiteral("画面尺寸"),ui->maxSizeBox);field(quality,QStringLiteral("视频码率"),row({ui->bitRateEdit,ui->bitRateBox}));
    ui->decodeModeBox->setItemText(0,QStringLiteral("通用解码"));if(ui->decodeModeBox->count()>1)ui->decodeModeBox->setItemText(1,QStringLiteral("硬件解码 · Apple Silicon"));field(quality,QStringLiteral("解码方式"),ui->decodeModeBox);
    rule(home);home->addWidget(label(QStringLiteral("声音"),"section"));auto *autoAudio=new QCheckBox(QStringLiteral("投屏时开启音频"));autoAudio->setChecked(QSettings().value(QStringLiteral("audio/startWithProjection"),false).toBool());connect(autoAudio,&QCheckBox::toggled,this,[](bool on){QSettings().setValue(QStringLiteral("audio/startWithProjection"),on);});home->addWidget(autoAudio);home->addWidget(label(QStringLiteral("将手机声音转发至电脑，手机仍可能播放声音。")));
    rule(home);ui->showToolbar->setText(QStringLiteral("显示快捷工具栏"));home->addWidget(ui->showToolbar);ui->showToolbar->show();home->addWidget(label(QStringLiteral("操作投屏时显示工具栏；闲置 5 秒或点击电脑其他区域时一起收起。拖动标题栏移动窗口，左上角和右下角可缩放。")));home->addStretch();
    auto *audio=layouts[1];audio->addWidget(label(QStringLiteral("音频转发"),"section"));audio->addWidget(label(QStringLiteral("将当前设备的声音传到电脑。此模式不是独占接管，手机扬声器可能继续播放。")));
    audio->addWidget(row({ui->startAudioBtn,ui->stopAudioBtn}));rule(audio);audio->addWidget(label(QStringLiteral("组件与诊断"),"section"));audio->addWidget(ui->installSndcpyBtn);ui->installSndcpyBtn->setText(QStringLiteral("准备手机音频组件"));ui->installSndcpyBtn->show();audio->addWidget(label(QStringLiteral("声音断续时，可在日志中查看音频欠载与连接情况。")));audio->addStretch();
    auto *mapping=layouts[2];mapping->addWidget(label(QStringLiteral("按键映射"),"section"));mapping->addWidget(ui->gameBox);ui->gameBox->show();mapping->addWidget(row({ui->refreshGameScriptBtn,ui->applyScriptBtn}));mapping->addWidget(label(QStringLiteral("先打开投屏并让窗口获得焦点。K80 配置使用反引号（` / ～）切换游戏操作模式。")));mapping->addStretch();
    auto *connections=layouts[3];connections->addWidget(label(QStringLiteral("设备连接"),"section"));connections->addWidget(row({ui->usbConnectBtn,ui->wifiConnectBtn}));ui->usbConnectBtn->setText(QStringLiteral("通过 USB 连接"));ui->wifiConnectBtn->setText(QStringLiteral("准备无线连接"));connections->addWidget(label(QStringLiteral("同一设备优先使用 USB。首次无线连接需先通过 USB 获取地址并启动调试。")));rule(connections);
    auto *wireless=section(connections,QStringLiteral("无线设置"));field(wireless,QStringLiteral("地址 / 端口"),row({ui->deviceIpEdt,ui->devicePortEdt}));connections->addWidget(row({ui->getIPBtn,ui->startAdbdBtn}));connections->addWidget(row({ui->wirelessConnectBtn,ui->wirelessDisConnectBtn}));rule(connections);auto *device=section(connections,QStringLiteral("设备备注"));field(device,QStringLiteral("名称"),row({ui->userNameEdt,ui->updateNameBtn}));connections->addStretch();
    auto *advanced=layouts[4];auto *details=new QTabWidget;auto *video=new QWidget;auto *vl=new QVBoxLayout(video);auto *vf=section(vl,QStringLiteral("视频与录制"));field(vf,QStringLiteral("视频来源"),ui->videoSourceBox);field(vf,QStringLiteral("摄像头"),row({ui->cameraFacingBox,ui->refreshCameraBtn}));field(vf,QStringLiteral("编码预设"),ui->codecModeBox);field(vf,QStringLiteral("屏幕方向"),ui->lockOrientationBox);field(vf,QStringLiteral("录像位置"),row({ui->recordPathEdt,ui->selectRecordPathBtn}));field(vf,QStringLiteral("录像格式"),ui->formatBox);vl->addWidget(row({ui->recordScreenCheck,ui->notDisplayCheck}));vl->addWidget(row({ui->alwaysTopCheck,ui->fpsCheck}));vl->addWidget(row({ui->closeScreenCheck,ui->stayAwakeCheck}));vl->addWidget(row({ui->framelessCheck,ui->useReverseCheck}));vl->addStretch();auto *videoScroll=new QScrollArea;videoScroll->setWidgetResizable(true);videoScroll->setFrameShape(QFrame::NoFrame);videoScroll->setWidget(video);details->addTab(videoScroll,QStringLiteral("投屏选项"));
    auto *preferences=new QWidget;initAdvancedConfigUi(preferences);details->addTab(preferences,QStringLiteral("应用偏好"));auto *display=new QWidget;auto *dl=new QVBoxLayout(display);dl->addWidget(m_advancedDisplayGroup);m_advancedDisplayGroup->show();dl->addStretch();auto *displayScroll=new QScrollArea;displayScroll->setWidgetResizable(true);displayScroll->setFrameShape(QFrame::NoFrame);displayScroll->setWidget(display);details->addTab(displayScroll,QStringLiteral("虚拟显示"));advanced->addWidget(details,1);
    // A dedicated diagnostic page is reached from the persistent footer.
    auto *logPage=new QWidget;logPage->setObjectName("Page");
    auto *ll=new QVBoxLayout(logPage);ll->setContentsMargins(0,10,8,12);ll->setSpacing(14);
    auto *logHeader=new QHBoxLayout;
    logHeader->addWidget(label(QStringLiteral("诊断日志"),"section"));logHeader->addStretch();
    ui->clearOut->setText(QStringLiteral("清空日志"));logHeader->addWidget(ui->clearOut);ui->clearOut->show();ll->addLayout(logHeader);
    ll->addWidget(label(QStringLiteral("查看设备连接、投屏和音频运行记录。清空仅影响当前显示。")));
    ui->outEdit->setObjectName("DiagnosticLog");ui->outEdit->setAccessibleName(QStringLiteral("运行日志"));
    ui->outEdit->setPlaceholderText(QStringLiteral("暂无运行记录"));ll->addWidget(ui->outEdit,1);ui->outEdit->show();
    rule(ll);ll->addWidget(label(QStringLiteral("ADB 命令"),"section"));
    ui->adbCommandEdt->setAccessibleName(QStringLiteral("ADB 命令"));ui->adbCommandEdt->setPlaceholderText(QStringLiteral("例如 devices"));
    ui->adbCommandBtn->setText(QStringLiteral("执行"));ui->stopAdbBtn->setText(QStringLiteral("终止命令"));
    ll->addWidget(row({ui->adbCommandEdt,ui->adbCommandBtn,ui->stopAdbBtn}));
    auto *sessionActions=new QHBoxLayout;sessionActions->addStretch();ui->stopAllServerBtn->setText(QStringLiteral("结束全部投屏"));sessionActions->addWidget(ui->stopAllServerBtn);ui->stopAllServerBtn->show();ll->addLayout(sessionActions);pages->addWidget(logPage);
    auto *actions=new QHBoxLayout;actions->addStretch();ui->startServerBtn->setObjectName("PrimaryButton");ui->startServerBtn->setText(QStringLiteral("打开投屏"));ui->startServerBtn->setMinimumWidth(155);ui->stopServerBtn->setText(QStringLiteral("结束投屏"));ui->stopServerBtn->setToolTip(QStringLiteral("结束当前设备的画面、音频和录制，应用继续在后台运行"));actions->addWidget(ui->startServerBtn);actions->addWidget(ui->stopServerBtn);ui->startServerBtn->show();ui->stopServerBtn->show();ml->addLayout(actions);
    auto *footer=new QFrame;footer->setObjectName("Footer");auto *fl=new QHBoxLayout(footer);fl->setContentsMargins(22,9,22,9);fl->addWidget(label(QStringLiteral("关闭主界面继续运行 · 完全退出请用菜单或 ⌘Q")));fl->addStretch();auto *logs=new QPushButton(QStringLiteral("查看日志"));logs->setObjectName("TextButton");fl->addWidget(logs);connect(logs,&QPushButton::clicked,this,[=](){pages->setCurrentIndex(5);group->setExclusive(false);for(auto *b:group->buttons())b->setChecked(false);group->setExclusive(true);});shell->addWidget(footer);
    on_refreshGameScriptBtn_clicked();m_workspaceReady=true;setMinimumSize(860,650);resize(1000,700);
    for(auto *w:{ui->leftWidget,ui->rightWidget}){w->setMinimumSize(0,0);w->hide();}
    for(auto *w:findChildren<QWidget*>())if(qobject_cast<QPushButton*>(w)||qobject_cast<QComboBox*>(w)||qobject_cast<QLineEdit*>(w))w->setMinimumHeight(34);
    update();
}
