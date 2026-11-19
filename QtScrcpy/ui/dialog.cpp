#include <QDebug>
#include <QAbstractItemView>
#include <QDesktopServices>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSizePolicy>
#include <QScreen>
#include <QScrollArea>
#include <QSet>
#include <QStandardPaths>
#include <QStyledItemDelegate>
#include <QSpinBox>
#include <QTabWidget>
#include <QTcpSocket>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>
#include <QUrl>

#include "config.h"
#include "dialog.h"
#include "ui_dialog.h"
#include "videoform.h"
#include "../groupcontroller/groupcontroller.h"

#ifdef Q_OS_WIN32
#include "../util/winutils.h"
#endif

QString s_keyMapPath = "";
QString s_defaultKeyMapPath = "";

namespace {
class ComboBoxItemDelegate final : public QStyledItemDelegate
{
public:
    explicit ComboBoxItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize hint = QStyledItemDelegate::sizeHint(option, index);
        hint.setHeight(qMax(hint.height(), 30));
        return hint;
    }
};
} // namespace

const QString &getKeyMapPath()
{
    if (s_keyMapPath.isEmpty()) {
        s_keyMapPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_KEYMAP_PATH"));
        if (s_keyMapPath.isEmpty()) {
            s_keyMapPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/keymap";
        }
        if (!QDir().mkpath(s_keyMapPath)) {
            qWarning() << "Failed to create keymap directory:" << s_keyMapPath;
        }
    }
    return s_keyMapPath;
}

const QString &getDefaultKeyMapPath()
{
    if (s_defaultKeyMapPath.isEmpty()) {
        s_defaultKeyMapPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_DEFAULT_KEYMAP_PATH"));
        if (s_defaultKeyMapPath.isEmpty()) {
            s_defaultKeyMapPath = QCoreApplication::applicationDirPath() + "/keymap";
        }
    }
    return s_defaultKeyMapPath;
}

void initializeKeyMapDirectory()
{
    const QDir defaultDir(getDefaultKeyMapPath());
    const QDir userDir(getKeyMapPath());
    if (!defaultDir.exists() || !userDir.exists()) {
        return;
    }

    const QFileInfoList defaults = defaultDir.entryInfoList(QStringList() << "*.json", QDir::Files);
    for (const QFileInfo &fileInfo : defaults) {
        const QString userFile = userDir.filePath(fileInfo.fileName());
        if (!QFileInfo::exists(userFile) && !QFile::copy(fileInfo.filePath(), userFile)) {
            qWarning() << "Failed to copy default keymap to:" << userFile;
        }
    }
}

Dialog::Dialog(QWidget *parent) : QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);
    initializeKeyMapDirectory();
    initUI();

    updateBootConfig(true);

    // Intel Mac 上如果配置文件遗留 decodeMode=1，强制重置为 0
    // （VideoToolbox 选项在 initUI() 中已被移除）
#if defined(Q_OS_MACOS) && !defined(__arm64__)
    if (ui->decodeModeBox->currentIndex() != 0) {
        ui->decodeModeBox->setCurrentIndex(0);
    }
#endif

    on_useSingleModeCheck_clicked();
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect availableGeometry = screen->availableGeometry();
        move(availableGeometry.x() + (availableGeometry.width() - width()) / 2,
             availableGeometry.y() + (availableGeometry.height() - height()) / 2);
    }
    on_updateDevice_clicked();

    connect(&m_autoUpdatetimer, &QTimer::timeout, this, &Dialog::on_updateDevice_clicked);
    if (ui->autoUpdatecheckBox->isChecked()) {
        m_autoUpdatetimer.start(5000);
    }

    connect(&m_adb, &qsc::AdbProcess::adbProcessResult, this, [this](qsc::AdbProcess::ADB_EXEC_RESULT processResult) {
        QString log = "";
        bool newLine = true;
        QStringList args = m_adb.arguments();

        switch (processResult) {
        case qsc::AdbProcess::AER_ERROR_START:
            break;
        case qsc::AdbProcess::AER_SUCCESS_START:
            log = "adb run";
            newLine = false;
            break;
        case qsc::AdbProcess::AER_ERROR_EXEC:
            //log = m_adb.getErrorOut();
            if (args.contains("ifconfig") && args.contains("wlan0")) {
                getIPbyIp();
            } else if (args.contains("connect")) {
                // adb connect 失败时退出码可能非0，同样交给统一处理
                handleConnectOutput(m_adb.getStdOut(), m_adb.getErrorOut(), m_lastConnectAddr);
            } else if (args.contains("pair")) {
                handlePairOutput(m_adb.getStdOut(), m_adb.getErrorOut());
            }
            break;
        case qsc::AdbProcess::AER_ERROR_MISSING_BINARY:
            log = "adb not found";
            break;
        case qsc::AdbProcess::AER_SUCCESS_EXEC:
            //log = m_adb.getStdOut();
            if (args.contains("devices")) {
                QStringList devices = m_adb.getDevicesSerialFromStdOut();
                ui->serialBox->clear();
                ui->connectedPhoneList->clear();
                for (auto &item : devices) {
                    ui->serialBox->addItem(item);
                    ui->connectedPhoneList->addItem(Config::getInstance().getNickName(item) + "-" + item);
                }
            } else if (args.contains("mdns") && args.contains("services")) {
                handleMdnsOutput(m_adb.getStdOut());
            } else if (args.contains("pair")) {
                handlePairOutput(m_adb.getStdOut(), m_adb.getErrorOut());
            } else if (args.contains("connect")) {
                handleConnectOutput(m_adb.getStdOut(), m_adb.getErrorOut(), m_lastConnectAddr);
            } else if (args.contains("show") && args.contains("wlan0")) {
                QString ip = m_adb.getDeviceIPFromStdOut();
                if (ip.isEmpty()) {
                    log = "ip not find, connect to wifi?";
                    break;
                }
                ui->deviceIpEdt->setEditText(ip);
            } else if (args.contains("ifconfig") && args.contains("wlan0")) {
                QString ip = m_adb.getDeviceIPFromStdOut();
                if (ip.isEmpty()) {
                    log = "ip not find, connect to wifi?";
                    break;
                }
                ui->deviceIpEdt->setEditText(ip);
            } else if (args.contains("ip -o a")) {
                QString ip = m_adb.getDeviceIPByIpFromStdOut();
                if (ip.isEmpty()) {
                    log = "ip not find, connect to wifi?";
                    break;
                }
                ui->deviceIpEdt->setEditText(ip);
            }
            break;
        }
        if (!log.isEmpty()) {
            outLog(log, newLine);
        }
    });

    m_hideIcon = new QSystemTrayIcon(this);
    m_hideIcon->setIcon(QIcon(":/image/tray/logo.png"));
    m_menu = new QMenu(this);
    m_quit = new QAction(this);
    m_showWindow = new QAction(this);
    m_showWindow->setText(tr("show"));
    m_quit->setText(tr("quit"));
    m_menu->addAction(m_showWindow);
    m_menu->addAction(m_quit);
    m_hideIcon->setContextMenu(m_menu);
    m_hideIcon->show();
    connect(m_showWindow, &QAction::triggered, this, &Dialog::show);
    connect(m_quit, &QAction::triggered, this, [this]() {
        m_hideIcon->hide();
        qApp->quit();
    });
    connect(m_hideIcon, &QSystemTrayIcon::activated, this, &Dialog::slotActivated);

    connect(&qsc::IDeviceManage::getInstance(), &qsc::IDeviceManage::deviceConnected, this, &Dialog::onDeviceConnected);
    connect(&qsc::IDeviceManage::getInstance(), &qsc::IDeviceManage::deviceDisconnected, this, &Dialog::onDeviceDisconnected);
}

Dialog::~Dialog()
{
    qDebug() << "~Dialog()";
    updateBootConfig(false);
    qsc::IDeviceManage::getInstance().disconnectAllDevice();
    delete ui;
}

void Dialog::initUI()
{
    setAttribute(Qt::WA_DeleteOnClose);
    //setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);

    setWindowTitle(Config::getInstance().getTitle());
#ifdef Q_OS_LINUX
    // Set window icon (inherits from application icon set in main.cpp)
    // If application icon was set, this will use it automatically
    if (!qApp->windowIcon().isNull()) {
        setWindowIcon(qApp->windowIcon());
    }
#endif

#ifdef Q_OS_WIN32
    WinUtils::setDarkBorderToWindow((HWND)this->winId(), true);
#endif

    ui->bitRateEdit->setValidator(new QIntValidator(1, 99999, this));

    ui->presetConfigBtn->setVisible(false);

    // Populate the codec-mode combo. Index 0 is the static "Default (Auto)"
    // item from the .ui; every registered encoder preset is appended below it.
    for (const EncoderPreset &preset : EncoderPresetRegistry::all()) {
        ui->codecModeBox->addItem(preset.displayName);
    }

    ui->maxSizeBox->addItem("640");
    ui->maxSizeBox->addItem("720");
    ui->maxSizeBox->addItem("1080");
    ui->maxSizeBox->addItem("1280");
    ui->maxSizeBox->addItem("1920");
    ui->maxSizeBox->addItem(tr("original"));

    ui->videoSourceBox->addItem(tr("display"));
    ui->videoSourceBox->addItem(tr("camera"));
    ui->cameraFacingBox->addItem(tr("back"));
    ui->cameraFacingBox->addItem(tr("front"));

    ui->formatBox->addItem("mp4");
    ui->formatBox->addItem("mkv");

    ui->lockOrientationBox->addItem(tr("no lock"));
    ui->lockOrientationBox->addItem("0");
    ui->lockOrientationBox->addItem("90");
    ui->lockOrientationBox->addItem("180");
    ui->lockOrientationBox->addItem("270");
    ui->lockOrientationBox->setCurrentIndex(0);

    ui->decodeModeBox->addItem(tr("FFmpeg + OpenGL (Universal Default)"));
    ui->decodeModeBox->addItem(tr("VideoToolbox + Metal (Apple Silicon Only)"));
    ui->decodeModeBox->setCurrentIndex(0);

#ifndef Q_OS_MACOS
    // 非 macOS：隐藏整个解码模式控件行
    ui->decodeModeLabel->hide();
    ui->decodeModeBox->hide();
#elif !defined(__arm64__)
    // Intel Mac：移除 VideoToolbox 选项，用户只看到 FFmpeg
    ui->decodeModeBox->removeItem(1);
#endif

    // 加载地址历史记录（只存主机IP，端口不缓存）
    loadAddrHistory();

    // 端口输入框配置（端口不做历史，见 loadPortHistory 里的说明）
    loadPortHistory();

    // 配对码只允许输入数字
    ui->pairCodeEdt->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{0,6}"), this));

    // 为deviceIpEdt添加右键菜单
    if (ui->deviceIpEdt->lineEdit()) {
        ui->deviceIpEdt->lineEdit()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(ui->deviceIpEdt->lineEdit(), &QWidget::customContextMenuRequested,
                this, &Dialog::showIpEditMenu);
    }
    initAdvancedDisplayUi();

    auto *configTabs = new QTabWidget(ui->rightWidget);
    auto *startConfigPage = new QWidget(configTabs);
    auto *advancedDisplayPage = new QWidget(configTabs);
    auto *advancedConfigPage = new QWidget(configTabs);
    auto *startConfigLayout = new QVBoxLayout(startConfigPage);
    auto *advancedDisplayLayout = new QVBoxLayout(advancedDisplayPage);
    startConfigLayout->setContentsMargins(0, 0, 0, 0);
    advancedDisplayLayout->setContentsMargins(0, 0, 0, 0);

    ui->verticalLayout_5->removeWidget(ui->adbGroupBox);
    ui->verticalLayout_5->removeWidget(ui->outEdit);
    ui->verticalLayout_6->removeWidget(ui->configGroupBox);
    ui->verticalLayout_6->removeWidget(ui->usbGroupBox);
    ui->verticalLayout_6->removeWidget(ui->wirelessGroupBox);
    ui->verticalLayout_6->removeWidget(m_advancedDisplayGroup);
    delete ui->verticalLayout_6->takeAt(0);

    ui->verticalLayout_5->addWidget(ui->usbGroupBox);
    ui->verticalLayout_5->addWidget(ui->wirelessGroupBox);
    startConfigLayout->addWidget(ui->configGroupBox);
    startConfigLayout->addWidget(ui->adbGroupBox);
    startConfigLayout->addWidget(ui->outEdit, 1);
    advancedDisplayLayout->addWidget(m_advancedDisplayGroup);
    advancedDisplayLayout->addStretch();
    initAdvancedConfigUi(advancedConfigPage);
    configTabs->addTab(startConfigPage, tr("Start Config"));
    configTabs->addTab(advancedDisplayPage, tr("Advanced Display"));
    configTabs->addTab(advancedConfigPage, tr("Advanced Config"));
    configTabs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    ui->verticalLayout_6->addWidget(configTabs);

    ui->leftWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    ui->leftWidget->setMinimumWidth(0);
    ui->horizontalLayout_11->setStretch(0, 1);
    ui->horizontalLayout_11->setStretch(1, 2);
    ui->horizontalLayout->setStretch(0, 1);
    ui->horizontalLayout->setStretch(2, 0);

    for (QComboBox *comboBox : findChildren<QComboBox *>()) {
        comboBox->view()->setItemDelegate(new ComboBoxItemDelegate(comboBox->view()));
    }

    QSizePolicy simpleModePolicy = ui->simpleGroupBox->sizePolicy();
    simpleModePolicy.setVerticalPolicy(QSizePolicy::Expanding);
    ui->simpleGroupBox->setSizePolicy(simpleModePolicy);

    QSizePolicy logPolicy = ui->outEdit->sizePolicy();
    logPolicy.setVerticalPolicy(QSizePolicy::Expanding);
    ui->outEdit->setSizePolicy(logPolicy);
    ui->verticalLayout_5->setStretch(0, 0);
    ui->verticalLayout_5->setStretch(1, 1);
    ui->verticalLayout_5->setStretch(2, 0);
    ui->verticalLayout_5->setStretch(3, 0);
    ui->verticalLayout_4->setStretch(2, 1);
    ui->verticalLayout_6->setStretch(0, 1);
}

void Dialog::initAdvancedDisplayUi()
{
    m_advancedDisplayGroup = new QGroupBox(tr("Advanced display"), this);
    m_advancedDisplayGroup->setCheckable(true);
    m_advancedDisplayGroup->setChecked(false);
    auto *layout = new QFormLayout(m_advancedDisplayGroup);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_displayModeBox = new QComboBox(m_advancedDisplayGroup);
    m_displayModeBox->addItem(tr("Primary display"));
    m_displayModeBox->addItem(tr("Existing display ID"));
    m_displayModeBox->addItem(tr("New virtual display"));
    layout->addRow(tr("Display mode"), m_displayModeBox);

    m_displayIdEdit = new QLineEdit(m_advancedDisplayGroup);
    m_displayIdEdit->setPlaceholderText("1");
    layout->addRow(tr("Display ID"), m_displayIdEdit);

    m_newDisplayEdit = new QLineEdit(m_advancedDisplayGroup);
    m_newDisplayEdit->setPlaceholderText("1920x1080/240");
    layout->addRow(tr("Virtual size / DPI"), m_newDisplayEdit);

    m_cropEdit = new QLineEdit(m_advancedDisplayGroup);
    m_cropEdit->setPlaceholderText("width:height:x:y");
    layout->addRow(tr("Crop"), m_cropEdit);

    m_flexDisplayCheck = new QCheckBox(tr("Resize virtual display with window"), m_advancedDisplayGroup);
    layout->addRow(m_flexDisplayCheck);
    m_displayImePolicyBox = new QComboBox(m_advancedDisplayGroup);
    m_displayImePolicyBox->addItem(tr("Server default"), "");
    m_displayImePolicyBox->addItem("local", "local");
    m_displayImePolicyBox->addItem("fallback", "fallback");
    m_displayImePolicyBox->addItem("hide", "hide");
    layout->addRow(tr("IME policy"), m_displayImePolicyBox);
    m_vdSystemDecorationsCheck = new QCheckBox(tr("Show system decorations"), m_advancedDisplayGroup);
    m_vdSystemDecorationsCheck->setChecked(true);
    layout->addRow(m_vdSystemDecorationsCheck);
    m_vdDestroyContentCheck = new QCheckBox(tr("Destroy content on close"), m_advancedDisplayGroup);
    m_vdDestroyContentCheck->setChecked(true);
    layout->addRow(m_vdDestroyContentCheck);
    m_keepActiveCheck = new QCheckBox(tr("Keep device active"), m_advancedDisplayGroup);
    layout->addRow(m_keepActiveCheck);
    auto *startAppWidget = new QWidget(m_advancedDisplayGroup);
    auto *startAppLayout = new QHBoxLayout(startAppWidget);
    startAppLayout->setContentsMargins(0, 0, 0, 0);
    m_startAppBox = new QComboBox(startAppWidget);
    m_startAppBox->setEditable(true);
    m_startAppBox->setInsertPolicy(QComboBox::NoInsert);
    m_startAppBox->setPlaceholderText("com.android.settings");
    m_startAppBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_refreshAppsBtn = new QPushButton(tr("refresh"), startAppWidget);
    m_refreshAppsBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_refreshAppsBtn->setFixedWidth(m_refreshAppsBtn->sizeHint().width());
    startAppLayout->addWidget(m_startAppBox);
    startAppLayout->addWidget(m_refreshAppsBtn);
    layout->addRow(tr("Start app"), startAppWidget);

    connect(m_displayModeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Dialog::updateAdvancedDisplayUi);
    connect(m_flexDisplayCheck, &QCheckBox::toggled, this, &Dialog::updateAdvancedDisplayUi);
    connect(m_refreshAppsBtn, &QPushButton::clicked, this, &Dialog::refreshApps);
    updateAdvancedDisplayUi();
}

void Dialog::initAdvancedConfigUi(QWidget *parent)
{
    auto *pageLayout = new QVBoxLayout(parent);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    auto *scrollArea = new QScrollArea(parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scrollArea);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    auto *group = new QGroupBox(tr("Application configuration"), content);
    auto *layout = new QFormLayout(group);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_configLanguageBox = new QComboBox(group);
    m_configLanguageBox->addItem(tr("Automatic"), "Auto");
    m_configLanguageBox->addItem("简体中文", "zh_CN");
    m_configLanguageBox->addItem("English", "en_US");
    m_configLanguageBox->addItem("日本語", "ja_JP");
    m_configLanguageBox->addItem("한국어", "ko_KR");
    layout->addRow(tr("Language"), m_configLanguageBox);

    m_configTitleEdit = new QLineEdit(group);
    layout->addRow(tr("Window title"), m_configTitleEdit);

    m_configPushPathEdit = new QLineEdit(group);
    m_configPushPathEdit->setPlaceholderText("/sdcard/");
    layout->addRow(tr("Push file path"), m_configPushPathEdit);

    m_configMaxFpsSpin = new QSpinBox(group);
    m_configMaxFpsSpin->setRange(0, 1000);
    m_configMaxFpsSpin->setSpecialValueText(tr("Unlimited"));
    layout->addRow(tr("Maximum FPS"), m_configMaxFpsSpin);

    m_configRenderExpiredCheck = new QCheckBox(tr("Render expired video frames"), group);
    layout->addRow(m_configRenderExpiredCheck);

    m_configOpenGlBox = new QComboBox(group);
    m_configOpenGlBox->addItem(tr("Automatic"), -1);
    m_configOpenGlBox->addItem(tr("Software OpenGL"), 0);
    m_configOpenGlBox->addItem(tr("OpenGL ES"), 1);
    m_configOpenGlBox->addItem(tr("Desktop OpenGL"), 2);
    m_configOpenGlBox->setToolTip(tr("Takes effect after restarting QtScrcpy."));
    layout->addRow(tr("OpenGL backend"), m_configOpenGlBox);

    m_configServerPathEdit = new QLineEdit(group);
    m_configServerPathEdit->setPlaceholderText("/data/local/tmp/scrcpy-server.jar");
    layout->addRow(tr("Device server path"), m_configServerPathEdit);

    m_configAdbPathEdit = new QLineEdit(group);
    m_configAdbPathEdit->setPlaceholderText(tr("Use bundled ADB"));
    layout->addRow(tr("ADB executable"), m_configAdbPathEdit);

    m_configCodecOptionsEdit = new QLineEdit(group);
    m_configCodecOptionsEdit->setPlaceholderText("profile=1,level=2");
    layout->addRow(tr("Codec options"), m_configCodecOptionsEdit);

    m_configCodecNameEdit = new QLineEdit(group);
    m_configCodecNameEdit->setPlaceholderText("OMX.qcom.video.encoder.avc");
    layout->addRow(tr("Codec name"), m_configCodecNameEdit);

    m_configLogLevelBox = new QComboBox(group);
    m_configLogLevelBox->addItem("verbose", "verbose");
    m_configLogLevelBox->addItem("debug", "debug");
    m_configLogLevelBox->addItem("info", "info");
    m_configLogLevelBox->addItem("warn", "warn");
    m_configLogLevelBox->addItem("error", "error");
    layout->addRow(tr("Log level"), m_configLogLevelBox);

    auto *buttons = new QHBoxLayout();
    auto *openConfigButton = new QPushButton(tr("Open Config Directory"), group);
    auto *openKeymapButton = new QPushButton(tr("Open Keymap Directory"), group);
    auto *saveButton = new QPushButton(tr("Save"), group);
    buttons->addWidget(openConfigButton);
    buttons->addWidget(openKeymapButton);
    buttons->addStretch();
    buttons->addWidget(saveButton);
    layout->addRow(buttons);

    contentLayout->addWidget(group);
    contentLayout->addStretch();
    scrollArea->setWidget(content);
    pageLayout->addWidget(scrollArea);

    connect(openConfigButton, &QPushButton::clicked, this, [this] {
        openUserDirectory(Config::getInstance().getConfigDirectory());
    });
    connect(openKeymapButton, &QPushButton::clicked, this, [this] {
        openUserDirectory(getKeyMapPath());
    });
    connect(saveButton, &QPushButton::clicked, this, &Dialog::saveAdvancedConfig);
    loadAdvancedConfig();
}

void Dialog::loadAdvancedConfig()
{
    Config &config = Config::getInstance();
    m_configLanguageBox->setCurrentIndex(qMax(0, m_configLanguageBox->findData(config.getLanguage())));
    m_configTitleEdit->setText(config.getTitle());
    m_configPushPathEdit->setText(config.getPushFilePath());
    m_configMaxFpsSpin->setValue(qBound(0, config.getMaxFps(), m_configMaxFpsSpin->maximum()));
    m_configRenderExpiredCheck->setChecked(config.getRenderExpiredFrames() != 0);
    m_configOpenGlBox->setCurrentIndex(qMax(0, m_configOpenGlBox->findData(config.getDesktopOpenGL())));
    m_configServerPathEdit->setText(config.getServerPath());
    m_configAdbPathEdit->setText(config.getAdbPath());
    m_configCodecOptionsEdit->setText(config.getCodecOptions());
    m_configCodecNameEdit->setText(config.getCodecName());
    m_configLogLevelBox->setCurrentIndex(qMax(0, m_configLogLevelBox->findData(config.getLogLevel())));
}

void Dialog::saveAdvancedConfig()
{
    QString pushPath = m_configPushPathEdit->text().trimmed();
    if (!pushPath.isEmpty() && !pushPath.endsWith('/')) {
        pushPath += '/';
    }

    QMap<QString, QVariant> values;
    values.insert("Language", m_configLanguageBox->currentData());
    values.insert("WindowTitle", m_configTitleEdit->text().trimmed());
    values.insert("PushFilePath", pushPath);
    values.insert("MaxFps", m_configMaxFpsSpin->value());
    values.insert("RenderExpiredFrames", m_configRenderExpiredCheck->isChecked() ? 1 : 0);
    values.insert("UseDesktopOpenGL", m_configOpenGlBox->currentData());
    values.insert("ServerPath", m_configServerPathEdit->text().trimmed());
    values.insert("AdbPath", m_configAdbPathEdit->text().trimmed());
    values.insert("CodecOptions", m_configCodecOptionsEdit->text().trimmed());
    values.insert("CodecName", m_configCodecNameEdit->text().trimmed());
    values.insert("LogLevel", m_configLogLevelBox->currentData());

    if (!Config::getInstance().updateCommonConfig(values)) {
        QMessageBox::warning(this, tr("Save failed"), tr("Unable to save config.ini."));
        return;
    }
    m_configPushPathEdit->setText(pushPath);
    QMessageBox::information(this, tr("Configuration saved"),
                             tr("The settings were saved to config.ini. Restart QtScrcpy for all changes to take effect."));
}

void Dialog::openUserDirectory(const QString &path)
{
    if (!QDir().mkpath(path) || !QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        QMessageBox::warning(this, tr("Open directory failed"), path);
    }
}

void Dialog::updateAdvancedDisplayUi()
{
    if (!m_displayModeBox) {
        return;
    }
    const bool existing = m_displayModeBox->currentIndex() == 1;
    const bool virtualDisplay = m_displayModeBox->currentIndex() == 2;
    m_displayIdEdit->setEnabled(existing);
    m_newDisplayEdit->setEnabled(virtualDisplay);
    m_flexDisplayCheck->setEnabled(virtualDisplay);
    if (!virtualDisplay) {
        m_flexDisplayCheck->setChecked(false);
    }
    m_displayImePolicyBox->setEnabled(virtualDisplay);
    m_vdSystemDecorationsCheck->setEnabled(virtualDisplay);
    m_vdDestroyContentCheck->setEnabled(virtualDisplay);
    m_cropEdit->setEnabled(!m_flexDisplayCheck->isChecked());
}

void Dialog::updateBootConfig(bool toView)
{
    if (toView) {
        UserBootConfig config = Config::getInstance().getUserBootConfig();

        if (config.bitRate == 0) {
            ui->bitRateBox->setCurrentText("Mbps");
        } else if (config.bitRate % 1000000 == 0) {
            ui->bitRateEdit->setText(QString::number(config.bitRate / 1000000));
            ui->bitRateBox->setCurrentText("Mbps");
        } else {
            ui->bitRateEdit->setText(QString::number(config.bitRate / 1000));
            ui->bitRateBox->setCurrentText("Kbps");
        }

        ui->maxSizeBox->setCurrentIndex(config.maxSizeIndex);
        ui->formatBox->setCurrentIndex(config.recordFormatIndex);
        ui->recordPathEdt->setText(config.recordPath);
        ui->lockOrientationBox->setCurrentIndex(config.lockOrientationIndex);
        ui->framelessCheck->setChecked(config.framelessWindow);
        ui->recordScreenCheck->setChecked(config.recordScreen);
        ui->notDisplayCheck->setChecked(config.recordBackground);
        ui->useReverseCheck->setChecked(config.reverseConnect);
        ui->fpsCheck->setChecked(config.showFPS);
        ui->alwaysTopCheck->setChecked(config.windowOnTop);
        ui->closeScreenCheck->setChecked(config.autoOffScreen);
        ui->stayAwakeCheck->setChecked(config.keepAlive);
        ui->useSingleModeCheck->setChecked(config.simpleMode);
        ui->autoUpdatecheckBox->setChecked(config.autoUpdateDevice);
        ui->showToolbar->setChecked(config.showToolbar);
        ui->quitOnCloseCheck->setChecked(config.quitOnClose);
        ui->decodeModeBox->setCurrentIndex(config.decodeMode);
        ui->codecModeBox->setCurrentIndex(config.codecModeIndex);

        // Restore encoder preset dialog state (if dialog was previously opened)
        if (m_presetDialog) {
            m_presetDialog->setLevel(config.presetLevel);
        }
        ui->videoSourceBox->setCurrentIndex(config.videoSource);
        ui->cameraFacingBox->setCurrentIndex(qBound(0, config.cameraFacing, 1));
        if (m_advancedDisplayGroup) {
            m_advancedDisplayGroup->setChecked(config.advancedDisplay);
            m_displayModeBox->setCurrentIndex(qBound(0, config.displayMode, 2));
            m_displayIdEdit->setText(config.displayId);
            m_newDisplayEdit->setText(config.newDisplay);
            m_cropEdit->setText(config.crop);
            m_flexDisplayCheck->setChecked(config.flexDisplay);
            m_displayImePolicyBox->setCurrentIndex(qMax(0, m_displayImePolicyBox->findData(config.displayImePolicy)));
            m_vdSystemDecorationsCheck->setChecked(config.vdSystemDecorations);
            m_vdDestroyContentCheck->setChecked(config.vdDestroyContent);
            m_keepActiveCheck->setChecked(config.keepActive);
            m_startAppBox->setEditText(config.startApp);
            updateAdvancedDisplayUi();
        }
        updateVideoSourceUi();
    } else {
        UserBootConfig config = Config::getInstance().getUserBootConfig();

        config.bitRate = getBitRate();
        config.maxSizeIndex = ui->maxSizeBox->currentIndex();
        config.recordFormatIndex = ui->formatBox->currentIndex();
        config.recordPath = ui->recordPathEdt->text();
        config.lockOrientationIndex = ui->lockOrientationBox->currentIndex();
        config.recordScreen = ui->recordScreenCheck->isChecked();
        config.recordBackground = ui->notDisplayCheck->isChecked();
        config.reverseConnect = ui->useReverseCheck->isChecked();
        config.showFPS = ui->fpsCheck->isChecked();
        config.windowOnTop = ui->alwaysTopCheck->isChecked();
        config.autoOffScreen = ui->closeScreenCheck->isChecked();
        config.framelessWindow = ui->framelessCheck->isChecked();
        config.keepAlive = ui->stayAwakeCheck->isChecked();
        config.simpleMode = ui->useSingleModeCheck->isChecked();
        config.autoUpdateDevice = ui->autoUpdatecheckBox->isChecked();
        config.showToolbar = ui->showToolbar->isChecked();
        config.quitOnClose = ui->quitOnCloseCheck->isChecked();
        config.decodeMode = ui->decodeModeBox->currentIndex();
        config.codecModeIndex = ui->codecModeBox->currentIndex();

        // Save encoder preset dialog state (if opened)
        if (m_presetDialog) {
            config.presetLevel = m_presetDialog->selectedLevel();
        }

        config.videoSource = ui->videoSourceBox->currentIndex();
        config.cameraFacing = qMin(ui->cameraFacingBox->currentIndex(), 1);
        if (m_advancedDisplayGroup) {
            config.advancedDisplay = m_advancedDisplayGroup->isChecked();
            config.displayMode = m_displayModeBox->currentIndex();
            config.displayId = m_displayIdEdit->text().trimmed();
            config.newDisplay = m_newDisplayEdit->text().trimmed();
            config.crop = m_cropEdit->text().trimmed();
            config.flexDisplay = m_flexDisplayCheck->isChecked();
            config.displayImePolicy = m_displayImePolicyBox->currentData().toString();
            config.vdSystemDecorations = m_vdSystemDecorationsCheck->isChecked();
            config.vdDestroyContent = m_vdDestroyContentCheck->isChecked();
            config.keepActive = m_keepActiveCheck->isChecked();
            config.startApp = m_startAppBox->currentData().toString();
            if (config.startApp.isEmpty()) {
                config.startApp = m_startAppBox->currentText().trimmed();
            }
        }

        // 保存当前无线地址到历史记录（完整 ip:port，避免IP与端口错配）
        QString currentAddr = ui->deviceIpEdt->currentText().trimmed();
        if (!currentAddr.isEmpty()) {
            saveAddrHistory(currentAddr);
        }

        Config::getInstance().setUserBootConfig(config);
    }
}

void Dialog::execAdbCmd()
{
    if (checkAdbRun()) {
        return;
    }
    QString cmd = ui->adbCommandEdt->text().trimmed();
    outLog("adb " + cmd, false);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    m_adb.execute(ui->serialBox->currentText().trimmed(), cmd.split(" ", Qt::SkipEmptyParts));
#else
    m_adb.execute(ui->serialBox->currentText().trimmed(), cmd.split(" ", QString::SkipEmptyParts));
#endif
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
    if (fileName.isEmpty()) {
        return "";
    }

    QFile loadFile(getKeyMapPath() + "/" + fileName);
    if (!loadFile.exists()) {
        loadFile.setFileName(getDefaultKeyMapPath() + "/" + fileName);
    }
    if (!loadFile.open(QIODevice::ReadOnly)) {
        outLog("open file failed:" + fileName, true);
        return "";
    }

    QString ret = loadFile.readAll();
    loadFile.close();
    return ret;
}

void Dialog::slotActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
#ifdef Q_OS_WIN32
        this->show();
#endif
        break;
    default:
        break;
    }
}

void Dialog::bringToFront()
{
    show();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();
}

void Dialog::closeEvent(QCloseEvent *event)
{
    if (ui->quitOnCloseCheck->isChecked()) {
        // The user opted out of the system tray: closing the window quits the app.
        // Ignore the event (instead of accepting it) so WA_DeleteOnClose does not
        // delete this dialog here; main() deletes it after the event loop returns.
        m_hideIcon->hide();
        event->ignore();
        qApp->quit();
        return;
    }

    this->hide();
    if (!Config::getInstance().getTrayMessageShown()) {
        Config::getInstance().setTrayMessageShown(true);
        m_hideIcon->showMessage(tr("Notice"),
                                tr("Hidden here!"),
                                QSystemTrayIcon::Information,
                                3000);
    }
    event->ignore();
}

void Dialog::on_updateDevice_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    outLog("update devices...", false);
    m_adb.execute("", QStringList() << "devices");
}

void Dialog::updateVideoSourceUi()
{
    const bool camera = ui->videoSourceBox->currentIndex() == qsc::VIDEO_SOURCE_CAMERA;
    ui->cameraFacingLabel->setEnabled(camera);
    ui->cameraFacingBox->setEnabled(camera);
    ui->refreshCameraBtn->setEnabled(camera);
    ui->closeScreenCheck->setEnabled(!camera);
    ui->stayAwakeCheck->setEnabled(!camera);
    ui->gameBox->setEnabled(!camera);
    ui->refreshGameScriptBtn->setEnabled(!camera);
    ui->applyScriptBtn->setEnabled(!camera);
    ui->installSndcpyBtn->setEnabled(!camera);
    ui->startAudioBtn->setEnabled(!camera);
    if (m_advancedDisplayGroup) {
        m_advancedDisplayGroup->setEnabled(!camera);
    }
}

void Dialog::on_videoSourceBox_currentIndexChanged(int)
{
    updateVideoSourceUi();
}

void Dialog::on_startServerBtn_clicked()
{
    outLog("start server...", false);

    // this is ok that "original" toUshort is 0
    quint16 videoSize = ui->maxSizeBox->currentText().trimmed().toUShort();
    qsc::DeviceParams params;
    params.serial = ui->serialBox->currentText().trimmed();
    params.maxSize = videoSize;
    params.bitRate = getBitRate();
    // on devices with Android >= 10, the capture frame rate can be limited
    params.maxFps = static_cast<quint32>(Config::getInstance().getMaxFps());
    params.videoSource = static_cast<qsc::VideoSource>(ui->videoSourceBox->currentIndex());
    params.cameraFacing = ui->cameraFacingBox->currentIndex() == 1
            ? qsc::CAMERA_FACING_FRONT
            : qsc::CAMERA_FACING_BACK;
    params.cameraId = ui->cameraFacingBox->currentData().toString();
    const bool camera = params.videoSource == qsc::VIDEO_SOURCE_CAMERA;
    params.closeScreen = !camera && ui->closeScreenCheck->isChecked();
    params.useReverse = ui->useReverseCheck->isChecked();
    params.display = !ui->notDisplayCheck->isChecked();
    params.renderExpiredFrames = Config::getInstance().getRenderExpiredFrames();
    if (ui->lockOrientationBox->currentIndex() > 0) {
        params.captureOrientationLock = 1;
        params.captureOrientation = (ui->lockOrientationBox->currentIndex() - 1) * 90;
    }
    // Camera sensors expose their video stream in landscape by default. Rotate
    // the default camera preview to portrait, while preserving an explicit
    // orientation selected by the user.
    if (camera && params.captureOrientationLock == 0) {
        params.captureOrientation = 90;
    }
    params.stayAwake = !camera && ui->stayAwakeCheck->isChecked();
    params.recordFile = ui->recordScreenCheck->isChecked();
    params.recordPath = ui->recordPathEdt->text().trimmed();
    params.recordFileFormat = ui->formatBox->currentText().trimmed();
    params.serverLocalPath = getServerPath();
    params.serverRemotePath = Config::getInstance().getServerPath();
    params.pushFilePath = Config::getInstance().getPushFilePath();
    params.gameScript = camera ? QString() : getGameScript(ui->gameBox->currentText());
    params.logLevel = Config::getInstance().getLogLevel();
    // Apply an encoder preset (when a preset mode is selected) or the ini default.
    const int codecModeIndex = ui->codecModeBox->currentIndex();
    if (codecModeIndex > 0) {
        const EncoderPreset &preset = EncoderPresetRegistry::all().at(codecModeIndex - 1);
        // Restore from the open dialog's selection, or fall back to the saved tier.
        int level = 1;
        if (m_presetDialog) {
            level = m_presetDialog->selectedLevel();
        } else {
            level = Config::getInstance().getUserBootConfig().presetLevel;
        }
        QString codecOptions;
        quint32 bitRate = 0;
        preset.buildParams(level, codecOptions, bitRate);
        params.codecOptions = codecOptions;
        if (!preset.codecName.isEmpty()) {
            params.codecName = preset.codecName;
        }
        if (bitRate > 0) {
            params.bitRate = bitRate;
        }
    } else {
        params.codecOptions = Config::getInstance().getCodecOptions();
        params.codecName = Config::getInstance().getCodecName();
    }
    params.scid = QRandomGenerator::global()->bounded(1, 10000) & 0x7FFFFFFF;
    params.decodeMode = ui->decodeModeBox->currentIndex();
    // Disabled widgets may remain checked from persisted settings. Advanced
    // display options are not valid for camera capture, so never copy them
    // into camera session parameters.
    if (!camera && m_advancedDisplayGroup && m_advancedDisplayGroup->isChecked()) {
        const int displayMode = m_displayModeBox->currentIndex();
        if (displayMode == 1) {
            bool ok = false;
            const int displayId = m_displayIdEdit->text().trimmed().toInt(&ok);
            if (!ok || displayId < 0) {
                outLog(tr("invalid display ID"));
                return;
            }
            params.displayId = displayId;
        } else if (displayMode == 2) {
            params.newDisplay = m_newDisplayEdit->text().trimmed();
            if (params.newDisplay.isEmpty()) {
                // Match scrcpy's flex default (1280x960 at 160 dpi) while
                // keeping the command-line parameter explicit.
                params.newDisplay = "1280x960/160";
            }
            params.flexDisplay = m_flexDisplayCheck->isChecked();
            params.vdSystemDecorations = m_vdSystemDecorationsCheck->isChecked();
            params.vdDestroyContent = m_vdDestroyContentCheck->isChecked();
            params.displayImePolicy = m_displayImePolicyBox->currentData().toString();
        }
        params.keepActive = m_keepActiveCheck->isChecked();
        params.startApp = m_startAppBox->currentData().toString();
        if (params.startApp.isEmpty()) {
            params.startApp = m_startAppBox->currentText().trimmed();
        }
        // scrcpy forbids crop with flex display. Preserve the entered value
        // for a later non-flex session, but never pass it to the server.
        params.crop = params.flexDisplay ? QString() : m_cropEdit->text().trimmed();
    }
    if (params.flexDisplay && (params.newDisplay.isEmpty() || !params.display || !params.crop.isEmpty())) {
        outLog(tr("flex display requires video, a new virtual display, and no crop"));
        return;
    }

    const bool needsVirtualDisplayCheck = !params.newDisplay.isEmpty();
    if (!camera && !needsVirtualDisplayCheck) {
        qsc::IDeviceManage::getInstance().connectDevice(params);
        return;
    }

    auto *versionAdb = new qsc::AdbProcess(this);
    connect(versionAdb, &qsc::AdbProcess::adbProcessResult, this,
            [this, versionAdb, camera, params](qsc::AdbProcess::ADB_EXEC_RESULT result) {
        if (result == qsc::AdbProcess::AER_SUCCESS_EXEC) {
            bool ok = false;
            const int sdk = versionAdb->getStdOut().trimmed().toInt(&ok);
            const int minimumSdk = camera ? 31 : 29;
            if (ok && sdk >= minimumSdk) {
                qsc::IDeviceManage::getInstance().connectDevice(params);
            } else {
                outLog(camera ? tr("camera preview requires Android 12 or later")
                              : tr("virtual display requires Android 10 or later"));
            }
            versionAdb->deleteLater();
        } else if (result == qsc::AdbProcess::AER_ERROR_EXEC
                   || result == qsc::AdbProcess::AER_ERROR_START
                   || result == qsc::AdbProcess::AER_ERROR_MISSING_BINARY) {
            outLog(tr("could not verify Android version for camera preview"));
            versionAdb->deleteLater();
        }
    });
    versionAdb->execute(params.serial, QStringList() << "shell" << "getprop" << "ro.build.version.sdk");
}

void Dialog::on_refreshCameraBtn_clicked()
{
    const QString serial = ui->serialBox->currentText().trimmed();
    if (serial.isEmpty()) {
        outLog(tr("no device"));
        return;
    }
    if (qsc::IDeviceManage::getInstance().getDevice(serial)) {
        outLog(tr("stop preview first"));
        return;
    }

    ui->refreshCameraBtn->setEnabled(false);
    ui->refreshCameraBtn->setText("...");

    auto *pushAdb = new qsc::AdbProcess(this);
    connect(pushAdb, &qsc::AdbProcess::adbProcessResult, this,
            [this, pushAdb, serial](qsc::AdbProcess::ADB_EXEC_RESULT result) {
        if (result == qsc::AdbProcess::AER_SUCCESS_EXEC) {
            pushAdb->deleteLater();

            auto *listAdb = new qsc::AdbProcess(this);
            connect(listAdb, &qsc::AdbProcess::adbProcessResult, this,
                    [this, listAdb](qsc::AdbProcess::ADB_EXEC_RESULT listResult) {
                if (listResult == qsc::AdbProcess::AER_SUCCESS_START) {
                    return;
                }
                ui->refreshCameraBtn->setText(tr("refresh"));
                ui->refreshCameraBtn->setEnabled(ui->videoSourceBox->currentIndex() == qsc::VIDEO_SOURCE_CAMERA);

                if (listResult != qsc::AdbProcess::AER_SUCCESS_EXEC) {
                    outLog(tr("camera refresh failed"));
                    listAdb->deleteLater();
                    return;
                }

                const QString output = listAdb->getStdOut() + '\n' + listAdb->getErrorOut();
                const QRegularExpression pattern(R"(--camera-id=(\S+)\s+\(([^,\)]+))");
                QRegularExpressionMatchIterator matches = pattern.globalMatch(output);

                const int facingIndex = ui->cameraFacingBox->currentIndex();
                ui->cameraFacingBox->clear();
                ui->cameraFacingBox->addItem(tr("back"));
                ui->cameraFacingBox->addItem(tr("front"));

                int cameraCount = 0;
                while (matches.hasNext()) {
                    const QRegularExpressionMatch match = matches.next();
                    const QString cameraId = match.captured(1);
                    const QString facing = match.captured(2).trimmed();
                    ui->cameraFacingBox->addItem(QString("%1 (%2)").arg(cameraId, facing), cameraId);
                    ++cameraCount;
                }

                ui->cameraFacingBox->setCurrentIndex(qMin(facingIndex, ui->cameraFacingBox->count() - 1));
                outLog(cameraCount ? tr("camera refreshed") : tr("no camera"));
                listAdb->deleteLater();
            });

            QStringList args;
            args << "shell";
            args << QString("CLASSPATH=%1").arg(Config::getInstance().getServerPath());
            args << "app_process" << "/" << "com.genymobile.scrcpy.Server" << "4.1";
            args << "list_cameras=true" << "log_level=info";
            listAdb->execute(serial, args);
        } else if (result == qsc::AdbProcess::AER_ERROR_EXEC
                   || result == qsc::AdbProcess::AER_ERROR_START
                   || result == qsc::AdbProcess::AER_ERROR_MISSING_BINARY) {
            ui->refreshCameraBtn->setText(tr("refresh"));
            ui->refreshCameraBtn->setEnabled(ui->videoSourceBox->currentIndex() == qsc::VIDEO_SOURCE_CAMERA);
            outLog(tr("camera refresh failed"));
            pushAdb->deleteLater();
        }
    });
    pushAdb->push(serial, getServerPath(), Config::getInstance().getServerPath());
}

void Dialog::refreshApps()
{
    const QString serial = ui->serialBox->currentText().trimmed();
    if (serial.isEmpty()) {
        outLog(tr("no device"));
        return;
    }
    if (qsc::IDeviceManage::getInstance().getDevice(serial)) {
        outLog(tr("stop server first"));
        return;
    }

    m_refreshAppsBtn->setEnabled(false);
    m_refreshAppsBtn->setText("...");

    auto restoreRefreshButton = [this]() {
        m_refreshAppsBtn->setText(tr("refresh"));
        m_refreshAppsBtn->setEnabled(ui->videoSourceBox->currentIndex() == qsc::VIDEO_SOURCE_DISPLAY);
    };

    auto *pushAdb = new qsc::AdbProcess(this);
    connect(pushAdb, &qsc::AdbProcess::adbProcessResult, this,
            [this, pushAdb, serial, restoreRefreshButton](qsc::AdbProcess::ADB_EXEC_RESULT result) {
        if (result == qsc::AdbProcess::AER_SUCCESS_EXEC) {
            pushAdb->deleteLater();

            auto *listAdb = new qsc::AdbProcess(this);
            connect(listAdb, &qsc::AdbProcess::adbProcessResult, this,
                    [this, listAdb, restoreRefreshButton](qsc::AdbProcess::ADB_EXEC_RESULT listResult) {
                if (listResult == qsc::AdbProcess::AER_SUCCESS_START) {
                    return;
                }
                restoreRefreshButton();

                if (listResult != qsc::AdbProcess::AER_SUCCESS_EXEC) {
                    outLog(tr("app refresh failed"));
                    listAdb->deleteLater();
                    return;
                }

                const QString selected = m_startAppBox->currentData().toString().isEmpty()
                        ? m_startAppBox->currentText().trimmed()
                        : m_startAppBox->currentData().toString();
                const QString output = listAdb->getStdOut() + '\n' + listAdb->getErrorOut();
                const QRegularExpression pattern(
                        R"(^\s*[\*\-]\s+(.+?)\s+([A-Za-z0-9_]+(?:\.[A-Za-z0-9_]+)+)\s*$)",
                        QRegularExpression::MultilineOption);
                QRegularExpressionMatchIterator matches = pattern.globalMatch(output);

                m_startAppBox->clear();
                m_startAppBox->addItem(QString(), QString());
                int appCount = 0;
                while (matches.hasNext()) {
                    const QRegularExpressionMatch match = matches.next();
                    const QString name = match.captured(1).trimmed();
                    const QString packageName = match.captured(2);
                    m_startAppBox->addItem(QString("%1 (%2)").arg(name, packageName), packageName);
                    ++appCount;
                }

                const int selectedIndex = m_startAppBox->findData(selected);
                if (selectedIndex >= 0) {
                    m_startAppBox->setCurrentIndex(selectedIndex);
                } else {
                    m_startAppBox->setEditText(selected);
                }
                outLog(appCount ? tr("apps refreshed") : tr("no launchable app"));
                listAdb->deleteLater();
            });

            QStringList args;
            args << "shell";
            args << QString("CLASSPATH=%1").arg(Config::getInstance().getServerPath());
            args << "app_process" << "/" << "com.genymobile.scrcpy.Server" << "4.1";
            args << "list_apps=true" << "log_level=info";
            listAdb->execute(serial, args);
        } else if (result == qsc::AdbProcess::AER_ERROR_EXEC
                   || result == qsc::AdbProcess::AER_ERROR_START
                   || result == qsc::AdbProcess::AER_ERROR_MISSING_BINARY) {
            restoreRefreshButton();
            outLog(tr("app refresh failed"));
            pushAdb->deleteLater();
        }
    });
    pushAdb->push(serial, getServerPath(), Config::getInstance().getServerPath());
}

void Dialog::on_stopServerBtn_clicked()
{
    if (qsc::IDeviceManage::getInstance().disconnectDevice(ui->serialBox->currentText().trimmed())) {
        outLog("stop server");
    }
}

void Dialog::on_wirelessConnectBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    // 手动点一次连接就是一次新的尝试，允许再走一轮"端口过期 -> 重新解析"
    m_portRetried = false;
    QString addr = ui->deviceIpEdt->currentText().trimmed();
    if (addr.isEmpty()) {
        outLog(tr("error: device ip is null"), false);
        return;
    }

    // IP输入框允许直接填入完整地址（ip:port），例如从手机无线调试页面直接抄下来的地址。
    // 只有在没有带端口时才去拼接端口输入框的值。
    if (!addr.contains(':')) {
        QString port = ui->devicePortEdt->currentText().trimmed();
        if (port.isEmpty()) {
            // 端口留空表示"自动"。旧实现会退回 placeholder 的 5555，而无线调试的端口
            // 每次开启都重新随机分配，那个默认值几乎必然连不上。这里改为现场问 mDNS。
            m_pendingConnectHost = addr;
            m_mdnsRecovered = false;
            outLog(tr("port is not specified, looking it up on the LAN..."), false);
            QStringList mdnsArgs;
            mdnsArgs << "mdns";
            mdnsArgs << "services";
            m_adb.execute("", mdnsArgs);
            return;
        }
        addr += ":";
        addr += port;
    }
    m_pendingConnectHost.clear();

    // 记录本次连接的完整地址，连接失败后据此做端口探测与原因归类
    m_lastConnectAddr = addr;

    // 保存完整地址历史（ip:port），避免IP与端口分别保存后错配
    saveAddrHistory(addr);

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

void Dialog::onDeviceConnected(bool success, const QString &serial, const QString &deviceName, const QSize &size)
{
    Q_UNUSED(deviceName);
    if (!success) {
        return;
    }
    auto videoForm = new VideoForm(ui->framelessCheck->isChecked(), Config::getInstance().getSkin(), ui->showToolbar->isChecked(), ui->decodeModeBox->currentIndex());
    videoForm->setSerial(serial);

    qsc::IDeviceManage::getInstance().getDevice(serial)->setUserData(static_cast<void*>(videoForm));
    qsc::IDeviceManage::getInstance().getDevice(serial)->registerDeviceObserver(videoForm);


    videoForm->showFPS(ui->fpsCheck->isChecked());

    if (ui->alwaysTopCheck->isChecked()) {
        videoForm->staysOnTop();
    }

#ifndef Q_OS_WIN32
    // must be show before updateShowSize
    videoForm->show();
#endif
    QString name = Config::getInstance().getNickName(serial);
    if (name.isEmpty()) {
        name = Config::getInstance().getTitle();
    }
    videoForm->setWindowTitle(name + "-" + serial);
    videoForm->updateShowSize(size);

    bool deviceVer = size.height() > size.width();
    QRect rc = Config::getInstance().getRect(serial);
    bool rcVer = rc.height() > rc.width();
    // same width/height rate
    if (rc.isValid() && (deviceVer == rcVer)) {
        // mark: resize is for fix setGeometry magneticwidget bug
        videoForm->resize(rc.size());
        videoForm->setGeometry(rc);
    }

#ifdef Q_OS_WIN32
    // windows是show太早可以看到resize的过程
    QTimer::singleShot(200, videoForm, [videoForm](){videoForm->show();});
#endif

    GroupController::instance().addDevice(serial);
}

void Dialog::onDeviceDisconnected(QString serial)
{
    GroupController::instance().removeDevice(serial);
    auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
    if (!device) {
        return;
    }
    auto data = device->getUserData();
    if (data) {
        VideoForm* vf = static_cast<VideoForm*>(data);
        qsc::IDeviceManage::getInstance().getDevice(serial)->deRegisterDeviceObserver(vf);
        vf->close();
        vf->deleteLater();
    }
}

void Dialog::on_wirelessDisConnectBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    QString addr = ui->deviceIpEdt->currentText().trimmed();
    outLog("wireless disconnect...", false);
    QStringList adbArgs;
    adbArgs << "disconnect";
    adbArgs << addr;
    m_adb.execute("", adbArgs);
}

void Dialog::on_discoverBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }
    // 手动点击「发现」：这是一轮新的发现流程，重置自愈状态
    m_pendingConnectHost.clear();
    m_mdnsRecovered = false;
    m_portRetried = false;
    outLog(tr("discovering devices on the LAN..."), false);
    QStringList adbArgs;
    adbArgs << "mdns";
    adbArgs << "services";
    m_adb.execute("", adbArgs);
}

void Dialog::on_pairBtn_clicked()
{
    if (checkAdbRun()) {
        return;
    }

    QString ip = ui->deviceIpEdt->currentText().trimmed();
    // IP输入框可能填的是完整地址（ip:port），配对时只取IP部分
    if (ip.contains(':')) {
        ip = ip.split(':').first();
    }
    if (ip.isEmpty()) {
        outLog(tr("error: device ip is null"), false);
        return;
    }

    QString pairPort = ui->pairPortEdt->text().trimmed();
    QString pairCode = ui->pairCodeEdt->text().trimmed();
    if (pairPort.isEmpty() || pairCode.isEmpty()) {
        outLog(tr("error: pairing port and pairing code are both required"), false);
        return;
    }

    outLog(tr("pairing..."), false);
    QStringList adbArgs;
    adbArgs << "pair";
    adbArgs << (ip + ":" + pairPort);
    adbArgs << pairCode;
    m_adb.execute("", adbArgs);
}

void Dialog::handleMdnsOutput(const QString &stdOut)
{
    // adb mdns services 的输出形如（一台设备一行）：
    // adb-3412702d-xgZLql	_adb-tls-connect._tcp.	192.168.9.65:39633
    static const QRegularExpression re(
        "_adb-tls-connect\\._tcp\\.[\\s]+(\\d{1,3}(?:\\.\\d{1,3}){3}:\\d{1,5})");

    QStringList addrs;
    QRegularExpressionMatchIterator it = re.globalMatch(stdOut);
    while (it.hasNext()) {
        addrs << it.next().captured(1);
    }

    // 情况一：这次查询是为了给指定主机补全端口（用户只填了 IP 就点了连接）
    // adb 的 mDNS 结果来自它自己的缓存，可能是旧的：手机重开无线调试或重新配对后端口
    // 会变，缓存里那条记录的端口已经没人监听，拿去连就是"目标计算机积极拒绝"。
    // 所以先用 TCP 探一次，确认端口真的在监听才拿它去连。
    auto applyAddr = [this](const QString &found) {
        const QStringList parts = found.split(':');
        bool ok = false;
        const quint16 port = (parts.size() == 2) ? parts.at(1).toUShort(&ok) : 0;
        if (ok && !probeTcp(parts.at(0), port)) {
            outLog(tr("discovered port is stale, ignoring: ") + found, false);
            return false;
        }
        outLog(tr("device found: ") + found, false);
        // 历史里只留主机：端口每次开启无线调试都会重随，存下来必然是过期的
        ui->deviceIpEdt->setEditText(parts.at(0));
        saveAddrHistory(parts.at(0));
        if (parts.size() == 2) {
            ui->devicePortEdt->setEditText(parts.at(1));
        }
        return true;
    };

    bool staleFound = false;
    if (!m_pendingConnectHost.isEmpty()) {
        for (const QString &found : addrs) {
            if (found.split(':').first() != m_pendingConnectHost) {
                continue;
            }
            if (!applyAddr(found)) {
                staleFound = true;
                break;
            }
            m_pendingConnectHost.clear();
            requestWirelessConnect();
            return;
        }
    } else if (!addrs.isEmpty()) {
        // 情况二：普通的设备发现，取第一台端口真的在监听的
        for (const QString &found : addrs) {
            if (applyAddr(found)) {
                // 延迟到本轮adb回调结束后再发起连接，避免同一个AdbProcess实例被重入
                requestWirelessConnect();
                return;
            }
            staleFound = true;
        }
    }

    // 没有拿到可用结果，有两种可能：
    // 1) 列表为空 —— adb server 的 mDNS 客户端在启动时加入多播组，之后网络变化
    //    （手机掉线重连、网卡 IP/接口索引变化）不会重新加入，于是永远返回空；
    // 2) 拿到的记录全过期 —— 缓存里的旧端口已经没人监听。
    // 两者都能靠重启一次 adb server 解决（mDNS 客户端与缓存一起重建）。
    // 有设备连着时不动 server，否则会直接掐断投屏会话。
    if ((addrs.isEmpty() || staleFound) && !m_mdnsRecovered && !hasConnectedDevice()) {
        m_mdnsRecovered = true;
        outLog(tr("no usable mdns result, restarting the adb server and retrying..."), false);
        recoverAdbMdns(0);
        return;
    }

    if (staleFound) {
        m_pendingConnectHost.clear();
        outLog(tr("the address discovered on the LAN is stale: wireless debugging may be off, "
                  "or the phone switched network"), false);
        return;
    }

    if (!m_pendingConnectHost.isEmpty()) {
        m_pendingConnectHost.clear();
        outLog(tr("port not found on the LAN. if the device uses the legacy tcpip mode, "
                  "fill 5555 in the port box"), false);
        return;
    }

    outLog(tr("no wireless device found, make sure wireless debugging is enabled "
              "and the phone is on the same network"), false);
}

void Dialog::handlePairOutput(const QString &stdOut, const QString &errorOut)
{
    QString output = stdOut + errorOut;
    if (output.contains("Successfully paired", Qt::CaseInsensitive)
        || output.contains("Already paired", Qt::CaseInsensitive)) {
        outLog(tr("paired successfully, connecting..."), false);
        // 配对端口与配对码都是一次性的：每次打开手机上的配对弹窗都会重随，留着只会误导
        ui->pairCodeEdt->clear();
        ui->pairPortEdt->clear();
        // 配对会重建手机的无线调试服务，连接端口往往跟着变。端口框里存的是配对前的
        // 旧端口，直接拿去连必然"目标计算机积极拒绝"。清空后按 IP 走 mDNS 现场解析；
        // 稍等一会儿再查，给手机端把新的服务广播出来。
        ui->devicePortEdt->setEditText(QString());
        QTimer::singleShot(800, this, [this]() {
            requestWirelessConnect();
        });
        return;
    }

    outLog(tr("pairing failed, reopen \"pair device with pairing code\" on the phone: "
              "the port and the code change every time the dialog is reopened"), false);
    QString detail = output.trimmed();
    if (!detail.isEmpty()) {
        outLog(detail);
    }
}

void Dialog::handleConnectOutput(const QString &stdOut, const QString &errorOut, const QString &addr)
{
    QString output = stdOut + errorOut;
    if (output.contains("connected to", Qt::CaseInsensitive)) {
        outLog(tr("connected."), false);
        m_portRetried = false;
        return;
    }

    // 连接失败分两种，原因完全不同，必须区分开
    QStringList parts = addr.split(':');
    bool ok = false;
    quint16 port = (parts.size() == 2) ? parts.at(1).toUShort(&ok) : 0;
    if (!ok) {
        outLog(tr("connect failed: invalid address ") + addr, false);
        return;
    }

    if (!probeTcp(parts.at(0), port)) {
        // 端口不通最常见的原因就是它已经过期（重开无线调试、重新配对后都会重随）。
        // 手里有 IP 就直接按 IP 重新问一次 mDNS，省掉用户再点「发现」这一步。
        // 只自动补一次；有设备正在投屏时不动，避免打断会话。
        if (!m_portRetried && !hasConnectedDevice()) {
            m_portRetried = true;
            outLog(tr("port is not reachable, it may have changed. looking up the current "
                      "port on the LAN..."), false);
            retryWithFreshPort(parts.at(0));
            return;
        }
        outLog(tr("connect failed: port unreachable. the address or port is stale, wireless "
                  "debugging is off, or the phone switched network"), false);
        outLog(tr("click \"discover\" to fetch the current address from the phone"), false);
        return;
    }

    // TCP 端口是通的，但 adb upgrade 到 TLS 后被手机拒绝 —— Android 11+ 的配对失效
    outLog(tr("connect failed: the phone rejected this computer's key, pairing is required"),
           false);
    outLog(tr("on the phone: developer options -> wireless debugging -> pair device with "
              "pairing code, then fill the port and code below"), false);
    ui->pairPortEdt->setFocus();
    ui->pairPortEdt->selectAll();
}

bool Dialog::probeTcp(const QString &host, quint16 port, int timeoutMs)
{
    QTcpSocket socket;
    socket.connectToHost(host, port);
    return socket.waitForConnected(timeoutMs);
}

void Dialog::requestWirelessConnect(int retry)
{
    if (m_adb.isRuning()) {
        if (retry <= 0) {
            outLog(tr("adb is busy, please click \"wireless connect\" again"), false);
            return;
        }
        // 上一条adb命令尚未收尾，稍后重试
        QTimer::singleShot(120, this, [this, retry]() {
            requestWirelessConnect(retry - 1);
        });
        return;
    }

    on_wirelessConnectBtn_clicked();
}

bool Dialog::hasConnectedDevice() const
{
    // serialBox 由 `adb devices` 的结果填充。有设备时不能重启 adb server，
    // 否则正在投屏的会话会被直接掐断。
    return ui->serialBox->count() > 0;
}

void Dialog::execAdbWhenIdle(const QStringList &args, int retry)
{
    if (m_adb.isRuning()) {
        if (retry <= 0) {
            outLog(tr("adb is busy, please try again"), false);
            return;
        }
        QTimer::singleShot(120, this, [this, args, retry]() {
            execAdbWhenIdle(args, retry - 1);
        });
        return;
    }
    m_adb.execute("", args);
}

void Dialog::recoverAdbMdns(int step)
{
    if (step == 0) {
        QStringList adbArgs;
        adbArgs << "kill-server";
        execAdbWhenIdle(adbArgs);
        // 等 5037 端口释放；kill-server 失败也继续走下一步
        QTimer::singleShot(800, this, [this]() {
            recoverAdbMdns(1);
        });
        return;
    }

    if (step == 1) {
        QStringList adbArgs;
        adbArgs << "start-server";
        execAdbWhenIdle(adbArgs);
        // 给新的 server 一点时间把它自己的 mDNS 客户端拉起来
        QTimer::singleShot(1500, this, [this]() {
            recoverAdbMdns(2);
        });
        return;
    }

    outLog(tr("adb server restarted, discovering again..."), false);
    QStringList adbArgs;
    adbArgs << "mdns";
    adbArgs << "services";
    execAdbWhenIdle(adbArgs);
}

void Dialog::retryWithFreshPort(const QString &host)
{
    // 端口框里现在是过期值，清掉后才能走"端口为空 -> 现场向 mDNS 解析"这条路
    ui->devicePortEdt->setEditText(QString());
    m_pendingConnectHost = host;
    m_mdnsRecovered = false;
    QStringList adbArgs;
    adbArgs << "mdns";
    adbArgs << "services";
    execAdbWhenIdle(adbArgs);
}

void Dialog::on_selectRecordPathBtn_clicked()
{
    QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;
    QString directory = QFileDialog::getExistingDirectory(this, tr("select path"), "", options);
    ui->recordPathEdt->setText(directory);
}

void Dialog::on_recordPathEdt_textChanged(const QString &arg1)
{
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
    qsc::IDeviceManage::getInstance().disconnectAllDevice();
}

void Dialog::on_refreshGameScriptBtn_clicked()
{
    ui->gameBox->clear();
    initializeKeyMapDirectory();
    const QDir userDir(getKeyMapPath());
    const QDir defaultDir(getDefaultKeyMapPath());
    if (!userDir.exists() && !defaultDir.exists()) {
        outLog("keymap directory not find", true);
        return;
    }
    QSet<QString> fileNames;
    const QList<QDir> directories {userDir, defaultDir};
    for (const QDir &dir : directories) {
        const QFileInfoList list = dir.entryInfoList(QStringList() << "*.json", QDir::Files | QDir::NoSymLinks);
        for (const QFileInfo &fileInfo : list) {
            if (!fileNames.contains(fileInfo.fileName())) {
                fileNames.insert(fileInfo.fileName());
                ui->gameBox->addItem(fileInfo.fileName());
            }
        }
    }
}

void Dialog::on_applyScriptBtn_clicked()
{
    auto curSerial = ui->serialBox->currentText().trimmed();
    auto device = qsc::IDeviceManage::getInstance().getDevice(curSerial);
    if (!device) {
        return;
    }

    device->updateScript(getGameScript(ui->gameBox->currentText()));
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

void Dialog::on_usbConnectBtn_clicked()
{
    on_stopAllServerBtn_clicked();
    delayMs(200);
    on_updateDevice_clicked();
    delayMs(200);

    int firstUsbDevice = findDeviceFromeSerialBox(false);
    if (-1 == firstUsbDevice) {
        qWarning() << "No use device is found!";
        return;
    }
    ui->serialBox->setCurrentIndex(firstUsbDevice);

    on_startServerBtn_clicked();
}

int Dialog::findDeviceFromeSerialBox(bool wifi)
{
    QString regStr = "\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\:([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])\\b";
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QRegExp regIP(regStr);
#else
    QRegularExpression regIP(regStr);
#endif
    for (int i = 0; i < ui->serialBox->count(); ++i) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        bool isWifi = regIP.exactMatch(ui->serialBox->itemText(i));
#else
        bool isWifi = regIP.match(ui->serialBox->itemText(i)).hasMatch();
#endif
        bool found = wifi ? isWifi : !isWifi;
        if (found) {
            return i;
        }
    }

    return -1;
}

void Dialog::on_wifiConnectBtn_clicked()
{
    on_stopAllServerBtn_clicked();
    delayMs(200);

    on_updateDevice_clicked();
    delayMs(200);

    int firstUsbDevice = findDeviceFromeSerialBox(false);
    if (-1 == firstUsbDevice) {
        qWarning() << "No use device is found!";
        return;
    }
    ui->serialBox->setCurrentIndex(firstUsbDevice);

    on_getIPBtn_clicked();
    delayMs(200);

    on_startAdbdBtn_clicked();
    delayMs(1000);

    on_wirelessConnectBtn_clicked();
    delayMs(2000);

    on_updateDevice_clicked();
    delayMs(200);

    int firstWifiDevice = findDeviceFromeSerialBox(true);
    if (-1 == firstWifiDevice) {
        qWarning() << "No wifi device is found!";
        return;
    }
    ui->serialBox->setCurrentIndex(firstWifiDevice);

    on_startServerBtn_clicked();
}

void Dialog::on_connectedPhoneList_itemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);
    ui->serialBox->setCurrentIndex(ui->connectedPhoneList->currentRow());
    on_startServerBtn_clicked();
}

void Dialog::on_updateNameBtn_clicked()
{
    if (ui->serialBox->count() != 0) {
        if (ui->userNameEdt->text().isEmpty()) {
            Config::getInstance().setNickName(ui->serialBox->currentText(), "Phone");
        } else {
            Config::getInstance().setNickName(ui->serialBox->currentText(), ui->userNameEdt->text());
        }

        on_updateDevice_clicked();

        qDebug() << "Update OK!";
    } else {
        qWarning() << "No device is connected!";
    }
}

void Dialog::on_useSingleModeCheck_clicked()
{
    if (ui->useSingleModeCheck->isChecked()) {
        ui->rightWidget->hide();
    } else {
        ui->rightWidget->show();
    }

    adjustSize();
}

void Dialog::on_serialBox_currentIndexChanged(const QString &arg1)
{
    ui->userNameEdt->setText(Config::getInstance().getNickName(arg1));
}

quint32 Dialog::getBitRate()
{
    return ui->bitRateEdit->text().trimmed().toUInt() *
            (ui->bitRateBox->currentText() == QString("Mbps") ? 1000000 : 1000);
}

const QString &Dialog::getServerPath()
{
    static QString serverPath;
    if (serverPath.isEmpty()) {
        serverPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_SERVER_PATH"));
        QFileInfo fileInfo(serverPath);
        if (serverPath.isEmpty() || !fileInfo.isFile()) {
            serverPath = QCoreApplication::applicationDirPath() + "/scrcpy-server";
        }
    }
    return serverPath;
}

void Dialog::on_startAudioBtn_clicked()
{
    if (ui->serialBox->count() == 0) {
        qWarning() << "No device is connected!";
        return;
    }

    m_audioOutput.start(ui->serialBox->currentText(), 28200);
}

void Dialog::on_stopAudioBtn_clicked()
{
    m_audioOutput.stop();
}

void Dialog::on_installSndcpyBtn_clicked()
{
    if (ui->serialBox->count() == 0) {
        qWarning() << "No device is connected!";
        return;
    }
    m_audioOutput.installonly(ui->serialBox->currentText(), 28200);
}

void Dialog::on_autoUpdatecheckBox_toggled(bool checked)
{
    if (checked) {
        m_autoUpdatetimer.start(5000);
    } else {
        m_autoUpdatetimer.stop();
    }
}

void Dialog::showIpEditMenu(const QPoint &pos)
{
    QMenu *menu = ui->deviceIpEdt->lineEdit()->createStandardContextMenu();
    menu->addSeparator();
    
    QAction *clearHistoryAction = new QAction(tr("Clear History"), menu);
    connect(clearHistoryAction, &QAction::triggered, this, [this]() {
        Config::getInstance().clearIpHistory();
        Config::getInstance().clearAddrHistory();
        loadAddrHistory();
    });
    
    menu->addAction(clearHistoryAction);
    menu->exec(ui->deviceIpEdt->lineEdit()->mapToGlobal(pos));
    delete menu;
}

void Dialog::loadPortHistory()
{
    // 端口不做历史记录：无线调试的端口每次开启都会重新随机分配，
    // 旧端口留着只会误导。端口留空时由 mDNS 现场补全（见 on_wirelessConnectBtn_clicked）。
    ui->devicePortEdt->clear();

    if (ui->devicePortEdt->lineEdit()) {
        ui->devicePortEdt->lineEdit()->setFrame(false);
        ui->devicePortEdt->lineEdit()->setTextMargins(0, 0, 0, 0);
        ui->devicePortEdt->lineEdit()->setMaxLength(5);
        ui->devicePortEdt->lineEdit()->setPlaceholderText(tr("auto"));
    }
}

void Dialog::loadAddrHistory()
{
    QStringList addrList = Config::getInstance().getAddrHistory();
    if (addrList.isEmpty()) {
        // 兼容旧版本：老数据只保存了IP，没有端口
        addrList = Config::getInstance().getIpHistory();
    }

    ui->deviceIpEdt->clear();
    ui->deviceIpEdt->addItems(addrList);

    if (ui->deviceIpEdt->lineEdit()) {
        ui->deviceIpEdt->lineEdit()->setFrame(false);
        ui->deviceIpEdt->lineEdit()->setTextMargins(0, 0, 0, 0);
        ui->deviceIpEdt->lineEdit()->setMaxLength(128);
        ui->deviceIpEdt->lineEdit()->setPlaceholderText("192.168.0.1");
    }
}

void Dialog::saveAddrHistory(const QString &addr)
{
    if (addr.isEmpty()) {
        return;
    }

    Config::getInstance().saveAddrHistory(addr);

    // 更新ComboBox
    loadAddrHistory();
    ui->deviceIpEdt->setCurrentText(addr);
}

void Dialog::on_codecModeBox_currentIndexChanged(int index)
{
    // Block signals while updating bitrate to avoid recursive calls
    ui->bitRateEdit->blockSignals(true);
    ui->bitRateBox->blockSignals(true);

    const EncoderPreset *preset = nullptr;
    if (index > 0) {
        preset = &EncoderPresetRegistry::all().at(index - 1);
    }

    if (index == 0) {
        // Default: restore previous settings
        if (m_prevBitRate == 0) {
            ui->bitRateEdit->setText("2");
            ui->bitRateBox->setCurrentText("Mbps");
        } else if (m_prevBitRate % 1000000 == 0) {
            ui->bitRateEdit->setText(QString::number(m_prevBitRate / 1000000));
            ui->bitRateBox->setCurrentText("Mbps");
        } else {
            ui->bitRateEdit->setText(QString::number(m_prevBitRate / 1000));
            ui->bitRateBox->setCurrentText("Kbps");
        }
        // Restore previous max size
        ui->maxSizeBox->setCurrentIndex(m_prevMaxSizeIndex);
    } else {
        // Preset mode: save current settings for Default-mode restoration
        m_prevBitRate = getBitRate();
        m_prevMaxSizeIndex = ui->maxSizeBox->currentIndex();

        // Sync bitrate & maxSize to match the saved encoder tier
        UserBootConfig config = Config::getInstance().getUserBootConfig();
        int level = config.presetLevel;
        quint32 bitrate = preset->levelBitRate(level);
        if (bitrate > 0) {
            if (bitrate % 1000000 == 0) {
                ui->bitRateEdit->setText(QString::number(bitrate / 1000000));
                ui->bitRateBox->setCurrentText("Mbps");
            } else {
                ui->bitRateEdit->setText(QString::number(bitrate / 1000));
                ui->bitRateBox->setCurrentText("Kbps");
            }
        }
        int maxSizeIdx = preset->levelMaxSizeIndex(level);
        if (maxSizeIdx >= 0) {
            ui->maxSizeBox->setCurrentIndex(maxSizeIdx);
        }
    }

    // Show the encoder settings button only for presets with adjustable tiers
    ui->presetConfigBtn->setVisible(preset && preset->levelCount() >= 2);

    ui->bitRateEdit->blockSignals(false);
    ui->bitRateBox->blockSignals(false);
}

void Dialog::on_presetConfigBtn_clicked()
{
    const int codecModeIndex = ui->codecModeBox->currentIndex();
    if (codecModeIndex <= 0) {
        return;
    }
    const EncoderPreset &preset = EncoderPresetRegistry::all().at(codecModeIndex - 1);

    // (Re)create the dialog when the selected preset changes.
    if (!m_presetDialog || m_presetDialog->presetId() != preset.id) {
        m_presetDialog = new PresetConfigDialog(preset, this);
        // Only restore the saved tier on first creation — subsequent opens
        // preserve the user's last selection in the dialog.
        UserBootConfig bootConfig = Config::getInstance().getUserBootConfig();
        m_presetDialog->setLevel(bootConfig.presetLevel);

        // Sync bitrate & maxSize UI in real-time as the user switches tiers.
        connect(m_presetDialog.data(), &PresetConfigDialog::levelChanged,
                this, &Dialog::syncPresetLevelToUi);
    }

    m_presetDialog->exec();

    // Sync after dialog closes — covers the case where the initial tier was
    // restored via setLevel() above and no levelChanged signal fired.
    syncPresetLevelToUi();
}

void Dialog::syncPresetLevelToUi()
{
    const int codecModeIndex = ui->codecModeBox->currentIndex();
    if (codecModeIndex <= 0 || !m_presetDialog) {
        return;
    }
    const EncoderPreset &preset = EncoderPresetRegistry::all().at(codecModeIndex - 1);
    int level = m_presetDialog->selectedLevel();

    // Update bitrate
    quint32 bitrate = preset.levelBitRate(level);
    if (bitrate > 0) {
        ui->bitRateEdit->blockSignals(true);
        ui->bitRateBox->blockSignals(true);
        if (bitrate % 1000000 == 0) {
            ui->bitRateEdit->setText(QString::number(bitrate / 1000000));
            ui->bitRateBox->setCurrentText("Mbps");
        } else {
            ui->bitRateEdit->setText(QString::number(bitrate / 1000));
            ui->bitRateBox->setCurrentText("Kbps");
        }
        ui->bitRateEdit->blockSignals(false);
        ui->bitRateBox->blockSignals(false);
    }

    // Update max size (only when the tier prescribes one)
    int maxSizeIdx = preset.levelMaxSizeIndex(level);
    if (maxSizeIdx >= 0) {
        ui->maxSizeBox->setCurrentIndex(maxSizeIdx);
    }
}
