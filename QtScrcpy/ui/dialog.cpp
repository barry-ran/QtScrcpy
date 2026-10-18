#include <QDebug>
#include <QAbstractItemView>
#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPlainTextEdit>
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
#include <QSaveFile>
#include <QSizePolicy>
#include <QScreen>
#include <QStorageInfo>
#include <QSysInfo>
#include <QScrollArea>
#include <QSet>
#include <QStandardPaths>
#include <QStyledItemDelegate>
#include <QSpinBox>
#include <QTabWidget>
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
bool validatePosition(const QJsonObject &object, const QString &field, int mappingIndex, QString *error)
{
    const QJsonValue value = object.value(field);
    if (!value.isObject()) {
        if (error) *error = QObject::tr("Mapping %1 needs a %2 position.").arg(mappingIndex).arg(field);
        return false;
    }
    const QJsonObject position = value.toObject();
    const QJsonValue x = position.value("x");
    const QJsonValue y = position.value("y");
    if (!x.isDouble() || !y.isDouble()
            || x.toDouble() < 0.0 || x.toDouble() > 1.0
            || y.toDouble() < 0.0 || y.toDouble() > 1.0) {
        if (error) *error = QObject::tr("Mapping %1 has an invalid %2 position; x and y must be between 0 and 1.")
                                 .arg(mappingIndex).arg(field);
        return false;
    }
    return true;
}

bool validateKeyValue(const QJsonObject &object, const QString &field, int mappingIndex,
                      QSet<QString> *usedKeys, QString *error)
{
    const QString key = object.value(field).toString().trimmed();
    if (!object.value(field).isString() || key.isEmpty()) {
        if (error) *error = QObject::tr("Mapping %1 needs a %2 key name.").arg(mappingIndex).arg(field);
        return false;
    }
    const QString canonicalKey = key.toCaseFolded();
    if (usedKeys && usedKeys->contains(canonicalKey)) {
        if (error) *error = QObject::tr("Mapping %1 repeats the key %2.").arg(mappingIndex).arg(key);
        return false;
    }
    if (usedKeys) usedKeys->insert(canonicalKey);
    return true;
}

bool validateKeymap(const QByteArray &json, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = QObject::tr("JSON error near byte %1: %2")
                         .arg(static_cast<qlonglong>(parseError.offset)).arg(parseError.errorString());
        }
        return false;
    }
    if (!document.isObject()) {
        if (error) *error = QObject::tr("The keymap root must be a JSON object.");
        return false;
    }

    const QJsonObject root = document.object();
    if (root.contains("switchKey")
            && (!root.value("switchKey").isString() || root.value("switchKey").toString().trimmed().isEmpty())) {
        if (error) *error = QObject::tr("switchKey must be a non-empty Qt key name.");
        return false;
    }
    if (!root.value("keyMapNodes").isArray()) {
        if (error) *error = QObject::tr("keyMapNodes must be an array.");
        return false;
    }

    const QJsonArray nodes = root.value("keyMapNodes").toArray();
    QSet<QString> supportedTypes;
    supportedTypes << "KMT_CLICK" << "KMT_CLICK_TWICE" << "KMT_CLICK_MULTI" << "KMT_DRAG" << "KMT_STEER_WHEEL";
    QSet<QString> usedKeys;
    for (int i = 0; i < nodes.size(); ++i) {
        const int mappingIndex = i + 1;
        if (!nodes.at(i).isObject()) {
            if (error) *error = QObject::tr("Mapping %1 must be an object.").arg(mappingIndex);
            return false;
        }
        const QJsonObject node = nodes.at(i).toObject();
        const QString type = node.value("type").toString();
        if (!supportedTypes.contains(type)) {
            if (error) *error = QObject::tr("Mapping %1 uses an unsupported type: %2").arg(mappingIndex).arg(type);
            return false;
        }
        if (node.contains("switchMap") && !node.value("switchMap").isBool()) {
            if (error) *error = QObject::tr("Mapping %1 switchMap must be true or false.").arg(mappingIndex);
            return false;
        }

        if (type == "KMT_STEER_WHEEL") {
            if (!validatePosition(node, "centerPos", mappingIndex, error)) return false;
            const QStringList directionKeys {"leftKey", "rightKey", "upKey", "downKey"};
            for (const QString &field : directionKeys) {
                if (!validateKeyValue(node, field, mappingIndex, &usedKeys, error)) return false;
            }
            const QStringList offsets {"leftOffset", "rightOffset", "upOffset", "downOffset"};
            for (const QString &field : offsets) {
                if (!node.value(field).isDouble() || node.value(field).toDouble() < 0.0) {
                    if (error) *error = QObject::tr("Mapping %1 needs a non-negative %2.").arg(mappingIndex).arg(field);
                    return false;
                }
            }
        } else {
            if (!validateKeyValue(node, "key", mappingIndex, &usedKeys, error)) return false;
            if (type == "KMT_CLICK" || type == "KMT_CLICK_TWICE") {
                if (!validatePosition(node, "pos", mappingIndex, error)) return false;
            } else if (type == "KMT_CLICK_MULTI") {
                const QJsonValue clickNodesValue = node.value("clickNodes");
                if (!clickNodesValue.isArray() || clickNodesValue.toArray().isEmpty()) {
                    if (error) *error = QObject::tr("Mapping %1 needs at least one click node.").arg(mappingIndex);
                    return false;
                }
                const QJsonArray clickNodes = clickNodesValue.toArray();
                for (int clickIndex = 0; clickIndex < clickNodes.size(); ++clickIndex) {
                    const QJsonObject clickNode = clickNodes.at(clickIndex).toObject();
                    if (!clickNodes.at(clickIndex).isObject() || !clickNode.value("delay").isDouble()
                            || clickNode.value("delay").toDouble() < 0.0
                            || !validatePosition(clickNode, "pos", mappingIndex, error)) {
                        if (error && error->isEmpty()) *error = QObject::tr("Mapping %1 click node %2 needs a non-negative delay and a valid position.")
                                                                 .arg(mappingIndex).arg(clickIndex + 1);
                        return false;
                    }
                }
            } else if (type == "KMT_DRAG") {
                if (!validatePosition(node, "startPos", mappingIndex, error)
                        || !validatePosition(node, "endPos", mappingIndex, error)) return false;
                if (node.contains("dragSpeed")
                        && (!node.value("dragSpeed").isDouble() || node.value("dragSpeed").toDouble() < 0.0
                            || node.value("dragSpeed").toDouble() > 1.0)) {
                    if (error) *error = QObject::tr("Mapping %1 dragSpeed must be between 0 and 1.").arg(mappingIndex);
                    return false;
                }
                if (node.contains("startDelay")
                        && (!node.value("startDelay").isDouble() || node.value("startDelay").toDouble() < 0.0)) {
                    if (error) *error = QObject::tr("Mapping %1 startDelay must be non-negative.").arg(mappingIndex);
                    return false;
                }
            }
        }
    }

    if (root.contains("mouseMoveMap")) {
        if (!root.value("mouseMoveMap").isObject()) {
            if (error) *error = QObject::tr("mouseMoveMap must be an object.");
            return false;
        }
        const QJsonObject mouseMoveMap = root.value("mouseMoveMap").toObject();
        if (!validatePosition(mouseMoveMap, "startPos", 0, error)) return false;
        if (mouseMoveMap.contains("speedRatio")
                && (!mouseMoveMap.value("speedRatio").isDouble() || mouseMoveMap.value("speedRatio").toDouble() <= 0.0)) {
            if (error) *error = QObject::tr("mouseMoveMap speedRatio must be positive.");
            return false;
        }
        for (const QString &field : QStringList {"speedRatioX", "speedRatioY"}) {
            if (mouseMoveMap.contains(field)
                    && (!mouseMoveMap.value(field).isDouble() || mouseMoveMap.value(field).toDouble() <= 0.0)) {
                if (error) *error = QObject::tr("mouseMoveMap %1 must be positive.").arg(field);
                return false;
            }
        }
        if (mouseMoveMap.contains("smallEyes")) {
            const QJsonObject smallEyes = mouseMoveMap.value("smallEyes").toObject();
            if (!mouseMoveMap.value("smallEyes").isObject()
                    || !validateKeyValue(smallEyes, "key", 0, nullptr, error)
                    || !validatePosition(smallEyes, "pos", 0, error)) return false;
        }
    }
    return true;
}

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
    connect(&m_wifiReconnectTimer, &QTimer::timeout, this, &Dialog::on_updateDevice_clicked);
    m_wifiReconnectTimer.setInterval(15000);
    if (Config::getInstance().getWifiAutoReconnect() && !Config::getInstance().getWifiReconnectTarget().isEmpty()) {
        m_wifiReconnectTimer.start();
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
            }
            break;
        case qsc::AdbProcess::AER_ERROR_MISSING_BINARY:
            log = "adb not found";
            break;
        case qsc::AdbProcess::AER_SUCCESS_EXEC:
            //log = m_adb.getStdOut();
            if (args.contains("devices")) {
                QStringList devices = m_adb.getDevicesSerialFromStdOut();
                QSet<QString> selectedSerials;
                for (QListWidgetItem *item : ui->connectedPhoneList->selectedItems()) {
                    selectedSerials.insert(item->data(Qt::UserRole).toString());
                }
                ui->serialBox->clear();
                ui->connectedPhoneList->clear();
                for (auto &item : devices) {
                    ui->serialBox->addItem(item);
                    auto *deviceItem = new QListWidgetItem(Config::getInstance().getNickName(item) + "-" + item,
                                                           ui->connectedPhoneList);
                    deviceItem->setData(Qt::UserRole, item);
                    deviceItem->setSelected(selectedSerials.contains(item));
                }
                const QString reconnectTarget = Config::getInstance().getWifiReconnectTarget();
                if (Config::getInstance().getWifiAutoReconnect() && !reconnectTarget.isEmpty()
                        && !devices.contains(reconnectTarget)
                        && QDateTime::currentMSecsSinceEpoch() - m_lastWifiReconnectAttemptMs > 15000) {
                    m_lastWifiReconnectAttemptMs = QDateTime::currentMSecsSinceEpoch();
                    QTimer::singleShot(0, this, [this, reconnectTarget] {
                        if (!m_adb.isRuning()) {
                            m_adb.execute(QString(), QStringList() << "connect" << reconnectTarget);
                        }
                    });
                }
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
    connect(ui->gameBox, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        const bool camera = ui->videoSourceBox->currentIndex() == qsc::VIDEO_SOURCE_CAMERA;
        ui->deleteGameScriptBtn->setEnabled(!camera && !text.trimmed().isEmpty());
    });

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

    // 加载IP历史记录
    loadIpHistory();

    // 加载端口历史记录
    loadPortHistory();

    on_refreshGameScriptBtn_clicked();

    ui->connectedPhoneList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_savedGroupBox = new QComboBox(ui->simpleGroupBox);
    m_savedGroupBox->setToolTip(tr("Saved synchronized-control groups"));
    m_savedGroupBox->addItems(Config::getInstance().getDeviceGroupNames());
    auto *groupRow = new QHBoxLayout();
    auto *saveGroupButton = new QPushButton(tr("Save group"), ui->simpleGroupBox);
    auto *applyGroupButton = new QPushButton(tr("Apply group"), ui->simpleGroupBox);
    groupRow->addWidget(m_savedGroupBox, 1);
    groupRow->addWidget(saveGroupButton);
    groupRow->addWidget(applyGroupButton);
    ui->verticalLayout_4->addLayout(groupRow);
    connect(saveGroupButton, &QPushButton::clicked, this, [this] {
        QStringList serials;
        const QList<QListWidgetItem *> selected = ui->connectedPhoneList->selectedItems();
        for (QListWidgetItem *item : selected) {
            const QString serial = item->data(Qt::UserRole).toString();
            if (!serial.isEmpty()) serials.append(serial);
        }
        if (serials.isEmpty()) {
            QMessageBox::information(this, tr("Save group"), tr("Select one or more devices first."));
            return;
        }
        bool accepted = false;
        const QString name = QInputDialog::getText(this, tr("Save group"), tr("Group name:"),
                                                   QLineEdit::Normal, QString(), &accepted).trimmed();
        if (!accepted || name.isEmpty()) return;
        Config::getInstance().saveDeviceGroup(name, serials);
        const int index = m_savedGroupBox->findText(name);
        if (index < 0) m_savedGroupBox->addItem(name);
        m_savedGroupBox->setCurrentText(name);
    });
    connect(applyGroupButton, &QPushButton::clicked, this, [this] {
        applySavedDeviceGroup(m_savedGroupBox->currentText());
    });

    auto *editorButton = new QPushButton(tr("Edit / validate"), ui->configWidget4);
    auto *saveProfileButton = new QPushButton(tr("Save profile"), ui->configWidget4);
    auto *loadProfileButton = new QPushButton(tr("Load profile"), ui->configWidget4);
    ui->horizontalLayout_8->addWidget(editorButton);
    ui->horizontalLayout_8->addWidget(saveProfileButton);
    ui->horizontalLayout_8->addWidget(loadProfileButton);
    connect(editorButton, &QPushButton::clicked, this, [this] {
        QString fileName = ui->gameBox->currentText().trimmed();
        if (fileName.isEmpty()) {
            bool accepted = false;
            fileName = QInputDialog::getText(this, tr("New keymap"), tr("Name (without .json):"),
                                             QLineEdit::Normal, QString(), &accepted).trimmed();
            if (!accepted || fileName.isEmpty()) return;
            fileName.replace(QRegularExpression("[^A-Za-z0-9_. -]"), "_");
            if (!fileName.endsWith(".json", Qt::CaseInsensitive)) fileName += ".json";
        }
        if (QFileInfo(fileName).fileName() != fileName) {
            QMessageBox::warning(this, tr("Invalid keymap name"), tr("Use a file name without a directory path."));
            return;
        }
        const QString path = QDir(getKeyMapPath()).filePath(fileName);
        QFile file(path);
        QString initial = "{\n  \"switchKey\": \"Key_QuoteLeft\",\n  \"keyMapNodes\": []\n}\n";
        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(this, tr("Open keymap failed"), path);
                return;
            }
            initial = QString::fromUtf8(file.readAll());
            file.close();
        }

        QDialog editor(this);
        editor.setWindowTitle(tr("Edit keymap - %1").arg(fileName));
        editor.resize(700, 520);
        auto *layout = new QVBoxLayout(&editor);
        auto *textEdit = new QPlainTextEdit(&editor);
        textEdit->setPlainText(initial);
        auto *status = new QLabel(&editor);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &editor);
        auto *saveButton = buttons->button(QDialogButtonBox::Save);
        layout->addWidget(textEdit, 1);
        layout->addWidget(status);
        layout->addWidget(buttons);
        auto validate = [textEdit, status, saveButton] {
            QString error;
            const bool valid = validateKeymap(textEdit->toPlainText().toUtf8(), &error);
            status->setText(valid ? QObject::tr("Valid keymap JSON and mapping structure.") : error);
            status->setStyleSheet(valid ? "color: #16833b" : "color: #c0392b");
            saveButton->setEnabled(valid);
        };
        connect(textEdit, &QPlainTextEdit::textChanged, &editor, validate);
        validate();
        connect(buttons, &QDialogButtonBox::rejected, &editor, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::accepted, &editor, [&editor, &file, textEdit, path, fileName, this] {
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QMessageBox::warning(&editor, tr("Save keymap failed"), path);
                return;
            }
            file.write(textEdit->toPlainText().toUtf8());
            file.close();
            on_refreshGameScriptBtn_clicked();
            ui->gameBox->setCurrentText(fileName);
            on_applyScriptBtn_clicked();
            editor.accept();
        });
        editor.exec();
    });
    connect(saveProfileButton, &QPushButton::clicked, this, &Dialog::saveSelectedDeviceProfile);
    connect(loadProfileButton, &QPushButton::clicked, this, [this] {
        loadDeviceProfile(ui->serialBox->currentText().trimmed());
    });

    if (ui->recordPathEdt->parentWidget() && ui->recordPathEdt->parentWidget()->layout()) {
        auto *pathLayout = qobject_cast<QHBoxLayout *>(ui->recordPathEdt->parentWidget()->layout());
        if (pathLayout) {
            m_recordSpaceLabel = new QLabel(ui->recordPathEdt->parentWidget());
            auto *openRecordFolderButton = new QPushButton(tr("Open"), ui->recordPathEdt->parentWidget());
            pathLayout->addWidget(m_recordSpaceLabel);
            pathLayout->addWidget(openRecordFolderButton);
            connect(openRecordFolderButton, &QPushButton::clicked, this, [this] {
                openUserDirectory(ui->recordPathEdt->text().trimmed());
            });
            connect(ui->recordPathEdt, &QLineEdit::textChanged, this, &Dialog::updateRecordingSpaceLabel);
            updateRecordingSpaceLabel(ui->recordPathEdt->text());
        }
    }

    // 为deviceIpEdt添加右键菜单
    if (ui->deviceIpEdt->lineEdit()) {
        ui->deviceIpEdt->lineEdit()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(ui->deviceIpEdt->lineEdit(), &QWidget::customContextMenuRequested,
                this, &Dialog::showIpEditMenu);
    }
    
    // 为devicePortEdt添加右键菜单
    if (ui->devicePortEdt->lineEdit()) {
        ui->devicePortEdt->lineEdit()->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(ui->devicePortEdt->lineEdit(), &QWidget::customContextMenuRequested,
                this, &Dialog::showPortEditMenu);
    }
    initAdvancedDisplayUi();
    connect(m_startAppBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        loadDeviceProfile(ui->serialBox->currentText().trimmed());
    });
    connect(m_startAppBox, &QComboBox::currentTextChanged, this, [this](const QString &) {
        loadDeviceProfile(ui->serialBox->currentText().trimmed());
    });

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

    m_configAutoReconnectCheck = new QCheckBox(tr("Reconnect to the last Wi-Fi device automatically"), group);
    layout->addRow(m_configAutoReconnectCheck);
    connect(m_configAutoReconnectCheck, &QCheckBox::toggled, this, [](bool checked) {
        Config::getInstance().setWifiReconnectTarget(checked, Config::getInstance().getWifiReconnectTarget());
    });
    connect(m_configAutoReconnectCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && !Config::getInstance().getWifiReconnectTarget().isEmpty()) {
            m_wifiReconnectTimer.start();
        } else {
            m_wifiReconnectTimer.stop();
        }
    });

    auto *buttons = new QHBoxLayout();
    auto *openConfigButton = new QPushButton(tr("Open Config Directory"), group);
    auto *openKeymapButton = new QPushButton(tr("Open Keymap Directory"), group);
    auto *copyDiagnosticsButton = new QPushButton(tr("Copy Diagnostics"), group);
    auto *saveButton = new QPushButton(tr("Save"), group);
    buttons->addWidget(openConfigButton);
    buttons->addWidget(openKeymapButton);
    buttons->addWidget(copyDiagnosticsButton);
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
    connect(copyDiagnosticsButton, &QPushButton::clicked, this, [this] {
        const QString adbPath = Config::getInstance().getAdbPath();
        const QString effectiveAdbPath = adbPath.trimmed().isEmpty()
                ? QDir(QCoreApplication::applicationDirPath()).filePath("adb.exe") : adbPath;
        int wifiCount = 0;
        for (int i = 0; i < ui->serialBox->count(); ++i) {
            if (ui->serialBox->itemText(i).contains(':')) ++wifiCount;
        }
        const QStorageInfo storage(ui->recordPathEdt->text().trimmed());
        const QString freeSpace = storage.isValid() && storage.isReady()
                ? QString::number(storage.bytesAvailable() / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " GiB"
                : tr("unavailable");
        const QString report = QStringList {
            QStringLiteral("QtScrcpy %1").arg(QCoreApplication::applicationVersion()),
            QStringLiteral("Qt %1").arg(QString::fromLatin1(qVersion())),
            QStringLiteral("OS %1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture()),
            QStringLiteral("ADB executable present: %1").arg(QFileInfo::exists(effectiveAdbPath) ? tr("yes") : tr("no")),
            QStringLiteral("Connected devices: %1 (Wi-Fi: %2)").arg(ui->serialBox->count()).arg(wifiCount),
            QStringLiteral("Keymap files: %1").arg(QDir(getKeyMapPath()).entryList(QStringList() << "*.json", QDir::Files).size()),
            QStringLiteral("Recording directory free space: %1").arg(freeSpace)
        }.join('\n');
        QApplication::clipboard()->setText(report);
        QMessageBox::information(this, tr("Diagnostics copied"), tr("A privacy-conscious diagnostics summary was copied to the clipboard."));
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
    m_configAutoReconnectCheck->setChecked(config.getWifiAutoReconnect());
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

        // 保存当前IP到历史记录
        QString currentIp = ui->deviceIpEdt->currentText().trimmed();
        if (!currentIp.isEmpty()) {
            saveIpHistory(currentIp);
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
    ui->deleteGameScriptBtn->setEnabled(!camera && !ui->gameBox->currentText().trimmed().isEmpty());
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
    const QString selectedKeymapFile = camera ? QString() : ui->gameBox->currentText().trimmed();
    params.gameScript = camera ? QString() : getGameScript(selectedKeymapFile);
    if (!camera && !params.gameScript.isEmpty()) {
        QString validationError;
        if (!validateKeymap(params.gameScript.toUtf8(), &validationError)) {
            QMessageBox::warning(this, tr("Invalid keymap"), validationError);
            return;
        }
    }
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
        m_gameScripts.insert(params.serial, params.gameScript);
        m_gameScriptFiles.insert(params.serial, selectedKeymapFile);
        qsc::IDeviceManage::getInstance().connectDevice(params);
        return;
    }

    auto *versionAdb = new qsc::AdbProcess(this);
    connect(versionAdb, &qsc::AdbProcess::adbProcessResult, this,
            [this, versionAdb, camera, params, selectedKeymapFile](qsc::AdbProcess::ADB_EXEC_RESULT result) {
        if (result == qsc::AdbProcess::AER_SUCCESS_EXEC) {
            bool ok = false;
            const int sdk = versionAdb->getStdOut().trimmed().toInt(&ok);
            const int minimumSdk = camera ? 31 : 29;
            if (ok && sdk >= minimumSdk) {
                m_gameScripts.insert(params.serial, params.gameScript);
                m_gameScriptFiles.insert(params.serial, selectedKeymapFile);
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
    QString addr = ui->deviceIpEdt->currentText().trimmed();
    if (addr.isEmpty()) {
        outLog("error: device ip is null", false);
        return;
    }

    if (!ui->devicePortEdt->currentText().isEmpty()) {
        addr += ":";
        addr += ui->devicePortEdt->currentText().trimmed();
    } else if (!ui->devicePortEdt->lineEdit()->placeholderText().isEmpty()) {
        addr += ":";
        addr += ui->devicePortEdt->lineEdit()->placeholderText().trimmed();
    } else {
        outLog("error: device port is null", false);
        return;
    }

    // 保存IP历史记录 - 只保存IP部分,不包含端口
    QString ip = addr.split(":").first();
    if (!ip.isEmpty()) {
        saveIpHistory(ip);
    }
    
    // 保存端口历史记录
    QString port = addr.split(":").last();
    if (!port.isEmpty() && port != ip) {
        savePortHistory(port);
    }
    Config::getInstance().setWifiReconnectTarget(Config::getInstance().getWifiAutoReconnect(), addr);
    if (Config::getInstance().getWifiAutoReconnect()) {
        m_wifiReconnectTimer.start();
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

void Dialog::onDeviceConnected(bool success, const QString &serial, const QString &deviceName, const QSize &size)
{
    Q_UNUSED(deviceName);
    if (!success) {
        m_gameScripts.remove(serial);
        m_gameScriptFiles.remove(serial);
        return;
    }
    auto videoForm = new VideoForm(ui->framelessCheck->isChecked(), Config::getInstance().getSkin(), ui->showToolbar->isChecked(), ui->decodeModeBox->currentIndex());
    videoForm->setSerial(serial);
    videoForm->setKeyOverlayScript(m_gameScripts.value(serial));
    connect(videoForm, &VideoForm::keymapSaveRequested, this,
            [this, videoForm, serial](const QString &script) {
        QString validationError;
        if (!validateKeymap(script.toUtf8(), &validationError)) {
            QMessageBox::warning(videoForm, tr("Invalid keymap"), validationError);
            return;
        }

        QString fileName = m_gameScriptFiles.value(serial).trimmed();
        if (fileName.isEmpty()) {
            bool accepted = false;
            fileName = QInputDialog::getText(videoForm, tr("Save new keymap"),
                                             tr("Name (without .json):"), QLineEdit::Normal,
                                             QString(), &accepted).trimmed();
            if (!accepted || fileName.isEmpty()) {
                return;
            }
            fileName.replace(QRegularExpression("[^A-Za-z0-9_. -]"), "_");
            if (!fileName.endsWith(".json", Qt::CaseInsensitive)) {
                fileName += ".json";
            }
        }
        if (QFileInfo(fileName).fileName() != fileName || fileName == "." || fileName == "..") {
            QMessageBox::warning(videoForm, tr("Invalid keymap name"),
                                 tr("Use a file name without a directory path."));
            return;
        }

        initializeKeyMapDirectory();
        const QString path = QDir(getKeyMapPath()).filePath(fileName);
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(videoForm, tr("Save keymap failed"), path);
            return;
        }
        const QByteArray bytes = script.toUtf8();
        if (file.write(bytes) != bytes.size() || !file.commit()) {
            QMessageBox::warning(videoForm, tr("Save keymap failed"), path);
            return;
        }

        m_gameScriptFiles.insert(serial, fileName);
        m_gameScripts.insert(serial, script);
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (device) {
            device->updateScript(script);
        }
        if (ui->gameBox->findText(fileName) < 0) {
            ui->gameBox->addItem(fileName);
        }
        if (ui->serialBox->currentText().trimmed() == serial) {
            ui->gameBox->setCurrentText(fileName);
            saveSelectedDeviceProfile();
        }
        videoForm->setKeyOverlayScript(script);
        outLog(tr("Saved visual keymap: %1").arg(fileName));
    });

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
    if (!m_activeDeviceGroup.isEmpty()) {
        applySavedDeviceGroup(m_activeDeviceGroup);
    }
}

void Dialog::onDeviceDisconnected(QString serial)
{
    m_gameScripts.remove(serial);
    m_gameScriptFiles.remove(serial);
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

void Dialog::on_deleteGameScriptBtn_clicked()
{
    const QString fileName = ui->gameBox->currentText().trimmed();
    if (fileName.isEmpty() || QFileInfo(fileName).fileName() != fileName) {
        return;
    }

    const QString userPath = QDir(getKeyMapPath()).filePath(fileName);
    const QString defaultPath = QDir(getDefaultKeyMapPath()).filePath(fileName);
    if (QFileInfo::exists(defaultPath)) {
        QMessageBox::information(this, tr("Delete keymap"),
                                 tr("%1 is a bundled keymap and cannot be deleted. Save a copy with a different name first.").arg(fileName));
        return;
    }
    if (!QFileInfo::exists(userPath)) {
        QMessageBox::information(this, tr("Delete keymap"),
                                 tr("Only keymaps saved in your user keymap folder can be deleted."));
        return;
    }

    if (QMessageBox::question(this, tr("Delete keymap"),
                              tr("Delete %1 permanently?").arg(fileName),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    if (!QFile::remove(userPath)) {
        QMessageBox::warning(this, tr("Delete keymap failed"), userPath);
        return;
    }

    const QStringList affectedSerials = m_gameScriptFiles.keys(fileName);
    for (const QString &serial : affectedSerials) {
        m_gameScriptFiles.remove(serial);
        m_gameScripts.remove(serial);
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device) {
            continue;
        }
        device->updateScript(QString());
        if (void *userData = device->getUserData()) {
            auto *videoForm = static_cast<VideoForm*>(userData);
            videoForm->setKeyOverlayScript(QString());
            videoForm->setKeyOverlayVisible(false);
        }
    }

    on_refreshGameScriptBtn_clicked();
    ui->gameBox->setCurrentIndex(-1);
    saveSelectedDeviceProfile();
    outLog(tr("Deleted keymap: %1").arg(fileName));
}

void Dialog::on_applyScriptBtn_clicked()
{
    auto curSerial = ui->serialBox->currentText().trimmed();
    auto device = qsc::IDeviceManage::getInstance().getDevice(curSerial);
    if (!device) {
        return;
    }

    const QString script = getGameScript(ui->gameBox->currentText());
    QString validationError;
    if (script.isEmpty() || !validateKeymap(script.toUtf8(), &validationError)) {
        QMessageBox::warning(this, tr("Invalid keymap"), script.isEmpty() ? tr("The selected keymap could not be read.") : validationError);
        return;
    }
    device->updateScript(script);
    m_gameScripts.insert(curSerial, script);
    m_gameScriptFiles.insert(curSerial, ui->gameBox->currentText().trimmed());
    auto data = device->getUserData();
    if (data) {
        static_cast<VideoForm*>(data)->setKeyOverlayScript(script);
    }
    saveSelectedDeviceProfile();
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
    loadDeviceProfile(arg1);
}

void Dialog::saveSelectedDeviceProfile()
{
    const QString serial = ui->serialBox->currentText().trimmed();
    if (serial.isEmpty()) {
        QMessageBox::information(this, tr("Device profile"), tr("Select a device first."));
        return;
    }
    QString appPackage = m_startAppBox ? m_startAppBox->currentData().toString().trimmed() : QString();
    if (appPackage.isEmpty() && m_startAppBox) {
        const QString appText = m_startAppBox->currentText().trimmed();
        const QRegularExpression packageSuffix("\\(([^()]+)\\)$");
        const QRegularExpressionMatch match = packageSuffix.match(appText);
        appPackage = match.hasMatch() ? match.captured(1) : appText;
    }

    QMap<QString, QVariant> profile;
    profile.insert("maxSizeIndex", ui->maxSizeBox->currentIndex());
    profile.insert("bitRate", getBitRate());
    profile.insert("codecModeIndex", ui->codecModeBox->currentIndex());
    profile.insert("presetLevel", m_presetDialog ? m_presetDialog->selectedLevel()
                                                  : Config::getInstance().getUserBootConfig().presetLevel);
    profile.insert("videoSource", ui->videoSourceBox->currentIndex());
    profile.insert("cameraFacing", ui->cameraFacingBox->currentIndex());
    profile.insert("orientationIndex", ui->lockOrientationBox->currentIndex());
    profile.insert("keymap", ui->gameBox->currentText().trimmed());
    Config::getInstance().setDeviceProfile(serial, appPackage, profile);
    outLog(tr("Saved profile for %1 on %2").arg(appPackage.isEmpty() ? tr("default app") : appPackage, serial));
}

void Dialog::loadDeviceProfile(const QString &serial)
{
    if (m_loadingDeviceProfile || serial.trimmed().isEmpty() || !m_startAppBox) {
        return;
    }
    QString appPackage = m_startAppBox->currentData().toString().trimmed();
    if (appPackage.isEmpty()) {
        const QString appText = m_startAppBox->currentText().trimmed();
        const QRegularExpressionMatch match = QRegularExpression("\\(([^()]+)\\)$").match(appText);
        appPackage = match.hasMatch() ? match.captured(1) : appText;
    }
    const QMap<QString, QVariant> profile = Config::getInstance().getDeviceProfile(serial, appPackage);
    if (profile.isEmpty()) {
        return;
    }

    m_loadingDeviceProfile = true;
    ui->maxSizeBox->setCurrentIndex(qBound(0, profile.value("maxSizeIndex", ui->maxSizeBox->currentIndex()).toInt(), ui->maxSizeBox->count() - 1));
    ui->videoSourceBox->setCurrentIndex(qBound(0, profile.value("videoSource", ui->videoSourceBox->currentIndex()).toInt(), ui->videoSourceBox->count() - 1));
    ui->cameraFacingBox->setCurrentIndex(qBound(0, profile.value("cameraFacing", ui->cameraFacingBox->currentIndex()).toInt(), ui->cameraFacingBox->count() - 1));
    ui->lockOrientationBox->setCurrentIndex(qBound(0, profile.value("orientationIndex", ui->lockOrientationBox->currentIndex()).toInt(), ui->lockOrientationBox->count() - 1));
    ui->codecModeBox->setCurrentIndex(qBound(0, profile.value("codecModeIndex", ui->codecModeBox->currentIndex()).toInt(), ui->codecModeBox->count() - 1));
    const quint32 bitRate = profile.value("bitRate", getBitRate()).toUInt();
    if (bitRate > 0 && bitRate % 1000000 == 0) {
        ui->bitRateBox->setCurrentText("Mbps");
        ui->bitRateEdit->setText(QString::number(bitRate / 1000000));
    } else if (bitRate > 0) {
        ui->bitRateBox->setCurrentText("Kbps");
        ui->bitRateEdit->setText(QString::number(bitRate / 1000));
    }
    const int presetLevel = qBound(0, profile.value("presetLevel", 1).toInt(), 2);
    if (m_presetDialog) m_presetDialog->setLevel(presetLevel);
    const QString keymap = profile.value("keymap").toString();
    if (!keymap.isEmpty()) ui->gameBox->setCurrentText(keymap);
    m_loadingDeviceProfile = false;
}

void Dialog::applySavedDeviceGroup(const QString &name)
{
    if (name.trimmed().isEmpty()) return;
    const QStringList serials = Config::getInstance().getDeviceGroup(name);
    if (serials.isEmpty()) return;
    m_activeDeviceGroup = name;
    QString hostSerial;
    for (const QString &serial : serials) {
        if (qsc::IDeviceManage::getInstance().getDevice(serial)) {
            hostSerial = serial;
            break;
        }
    }
    for (int i = 0; i < ui->serialBox->count(); ++i) {
        const QString serial = ui->serialBox->itemText(i);
        auto device = qsc::IDeviceManage::getInstance().getDevice(serial);
        if (!device || !device->getUserData()) continue;
        const bool member = serials.contains(serial);
        const bool isHost = !member || serial == hostSerial;
        static_cast<VideoForm *>(device->getUserData())->setGroupHost(isHost);
    }
    outLog(tr("Applied synchronized-control group: %1").arg(name));
}

void Dialog::updateRecordingSpaceLabel(const QString &path)
{
    if (!m_recordSpaceLabel) return;
    const QStorageInfo storage(path.trimmed());
    if (path.trimmed().isEmpty() || !storage.isValid() || !storage.isReady()) {
        m_recordSpaceLabel->setText(tr("free space unavailable"));
        m_recordSpaceLabel->setToolTip(path.trimmed());
        return;
    }
    const double gib = static_cast<double>(storage.bytesAvailable()) / (1024.0 * 1024.0 * 1024.0);
    m_recordSpaceLabel->setText(tr("%1 GiB free").arg(gib, 0, 'f', 1));
    m_recordSpaceLabel->setToolTip(QDir::toNativeSeparators(path.trimmed()));
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

void Dialog::loadIpHistory()
{
    QStringList ipList = Config::getInstance().getIpHistory();
    ui->deviceIpEdt->clear();
    ui->deviceIpEdt->addItems(ipList);

    if (ui->deviceIpEdt->lineEdit()) {
        ui->deviceIpEdt->lineEdit()->setFrame(false);
        ui->deviceIpEdt->lineEdit()->setTextMargins(0, 0, 0, 0);
        ui->deviceIpEdt->lineEdit()->setMaxLength(128);
        ui->deviceIpEdt->lineEdit()->setPlaceholderText("192.168.0.1");
    }
}

void Dialog::saveIpHistory(const QString &ip)
{
    if (ip.isEmpty()) {
        return;
    }
    
    Config::getInstance().saveIpHistory(ip);
    
    // 更新ComboBox
    loadIpHistory();
    ui->deviceIpEdt->setCurrentText(ip);
}

void Dialog::showIpEditMenu(const QPoint &pos)
{
    QMenu *menu = ui->deviceIpEdt->lineEdit()->createStandardContextMenu();
    menu->addSeparator();
    
    QAction *clearHistoryAction = new QAction(tr("Clear History"), menu);
    connect(clearHistoryAction, &QAction::triggered, this, [this]() {
        Config::getInstance().clearIpHistory();
        loadIpHistory();
    });
    
    menu->addAction(clearHistoryAction);
    menu->exec(ui->deviceIpEdt->lineEdit()->mapToGlobal(pos));
    delete menu;
}

void Dialog::loadPortHistory()
{
    QStringList portList = Config::getInstance().getPortHistory();
    ui->devicePortEdt->clear();
    ui->devicePortEdt->addItems(portList);

    if (ui->devicePortEdt->lineEdit()) {
        ui->devicePortEdt->lineEdit()->setFrame(false);
        ui->devicePortEdt->lineEdit()->setTextMargins(0, 0, 0, 0);
        ui->devicePortEdt->lineEdit()->setMaxLength(6);
        ui->devicePortEdt->lineEdit()->setPlaceholderText("5555");
    }
}

void Dialog::savePortHistory(const QString &port)
{
    if (port.isEmpty()) {
        return;
    }
    
    Config::getInstance().savePortHistory(port);
    
    // 更新ComboBox
    loadPortHistory();
    ui->devicePortEdt->setCurrentText(port);
}

void Dialog::showPortEditMenu(const QPoint &pos)
{
    QMenu *menu = ui->devicePortEdt->lineEdit()->createStandardContextMenu();
    menu->addSeparator();
    
    QAction *clearHistoryAction = new QAction(tr("Clear History"), menu);
    connect(clearHistoryAction, &QAction::triggered, this, [this]() {
        Config::getInstance().clearPortHistory();
        loadPortHistory();
    });
    
    menu->addAction(clearHistoryAction);
    menu->exec(ui->devicePortEdt->lineEdit()->mapToGlobal(pos));
    delete menu;
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
