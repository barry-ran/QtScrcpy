#include "keymapdialog.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QUrl>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "QtScrcpyCore.h"

// ---------------------------------------------------------------------------
// Helpers: keymap paths  (mirrors dialog.cpp logic)
// ---------------------------------------------------------------------------
QString KeymapDialog::getUserKeymapPath() const
{
    QString p = QString::fromLocal8Bit(qgetenv("QTSCRCPY_KEYMAP_PATH"));
    if (p.isEmpty())
        p = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/keymap";
    QDir().mkpath(p);
    return p;
}

QString KeymapDialog::getDefaultKeymapPath() const
{
    QString p = QString::fromLocal8Bit(qgetenv("QTSCRCPY_DEFAULT_KEYMAP_PATH"));
    if (p.isEmpty())
        p = QCoreApplication::applicationDirPath() + "/keymap";
    return p;
}

/** يحاول إيجاد الملف أولاً في مجلد المستخدم، ثم في المجلد الافتراضي */
QString KeymapDialog::resolveKeymapPath(const QString &fileName) const
{
    QString userPath = getUserKeymapPath() + "/" + fileName;
    if (QFile::exists(userPath))
        return userPath;
    return getDefaultKeymapPath() + "/" + fileName;
}

/** تحويل نوع الـ key إلى نص عربي / إنجليزي مقروء */
QString KeymapDialog::keyTypeToString(const QString &type) const
{
    if (type == "KMT_CLICK")       return tr("Click");
    if (type == "KMT_STEER_WHEEL") return tr("Steer Wheel");
    if (type == "KMT_MOUSE_MOVE")  return tr("Mouse Move");
    if (type == "KMT_CLICK_TWICE") return tr("Double Click");
    if (type == "KMT_CLICK_MULTI") return tr("Multi Click");
    return type;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
KeymapDialog::KeymapDialog(const QString &serial, QWidget *parent)
    : QDialog(parent), m_serial(serial)
{
    setWindowTitle(tr("🎮 Keymap Manager"));
    setMinimumSize(860, 560);
    resize(960, 620);
    setupUI();
    refreshFileList();
}

KeymapDialog::~KeymapDialog() {}

// ---------------------------------------------------------------------------
// UI Setup
// ---------------------------------------------------------------------------
void KeymapDialog::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 6);
    mainLayout->setSpacing(8);

    // ── Title bar info ──────────────────────────────────────────────────────
    auto *titleBar = new QHBoxLayout();
    auto *titleLabel = new QLabel(tr("<b>Keymap Manager</b> — device: <code>%1</code>").arg(m_serial));
    titleLabel->setStyleSheet("font-size: 13px; color: #ddd;");
    titleBar->addWidget(titleLabel);
    titleBar->addStretch();

    m_statusLabel = new QLabel(tr("Select a keymap to preview or apply."));
    m_statusLabel->setStyleSheet("color: #aaa; font-style: italic;");
    titleBar->addWidget(m_statusLabel);
    mainLayout->addLayout(titleBar);

    // ── Horizontal splitter: file list | details/editor ────────────────────
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(4);

    // ── LEFT: File list panel ───────────────────────────────────────────────
    auto *leftWidget = new QWidget();
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto *listLabel = new QLabel(tr("📋 Keymap Files"));
    listLabel->setStyleSheet("font-weight: bold; color: #ccc; padding: 2px 0;");
    leftLayout->addWidget(listLabel);

    m_fileList = new QListWidget();
    m_fileList->setAlternatingRowColors(true);
    m_fileList->setStyleSheet(
        "QListWidget { background:#1e1e2e; color:#cdd6f4; border:1px solid #45475a; border-radius:4px; }"
        "QListWidget::item { padding: 6px 8px; }"
        "QListWidget::item:selected { background:#313244; color:#cba6f7; }"
        "QListWidget::item:hover { background:#2a2a3e; }");
    leftLayout->addWidget(m_fileList);

    // Left action buttons
    auto *leftBtns = new QHBoxLayout();
    m_newBtn    = new QPushButton(tr("＋ New"));
    m_deleteBtn = new QPushButton(tr("🗑 Delete"));
    m_newBtn->setFixedHeight(28);
    m_deleteBtn->setFixedHeight(28);
    m_newBtn->setStyleSheet("QPushButton{background:#313244;color:#a6e3a1;border:1px solid #45475a;border-radius:3px;}"
                            "QPushButton:hover{background:#45475a;}");
    m_deleteBtn->setStyleSheet("QPushButton{background:#313244;color:#f38ba8;border:1px solid #45475a;border-radius:3px;}"
                               "QPushButton:hover{background:#45475a;}");
    leftBtns->addWidget(m_newBtn);
    leftBtns->addWidget(m_deleteBtn);
    leftLayout->addLayout(leftBtns);

    leftWidget->setMinimumWidth(200);
    leftWidget->setMaximumWidth(260);
    splitter->addWidget(leftWidget);

    // ── RIGHT: Tab widget (Details | JSON Editor) ──────────────────────────
    m_tabWidget = new QTabWidget();
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border:1px solid #45475a; background:#1e1e2e; border-radius:4px; }"
        "QTabBar::tab { background:#313244; color:#cdd6f4; padding:6px 16px; border-radius:3px 3px 0 0; }"
        "QTabBar::tab:selected { background:#45475a; color:#cba6f7; }");

    // -- Tab 1: Details view --------------------------------------------------
    auto *detailsWidget = new QWidget();
    auto *detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(8, 8, 8, 8);
    detailsLayout->setSpacing(6);

    m_switchKeyLabel = new QLabel(tr("Switch Key: —"));
    m_switchKeyLabel->setStyleSheet("color:#f9e2af; font-weight:bold; font-size:12px;");
    detailsLayout->addWidget(m_switchKeyLabel);

    m_filePathLabel = new QLabel();
    m_filePathLabel->setStyleSheet("color:#6c7086; font-size:10px;");
    m_filePathLabel->setWordWrap(true);
    detailsLayout->addWidget(m_filePathLabel);

    m_detailsTable = new QTableWidget(0, 4);
    m_detailsTable->setHorizontalHeaderLabels({tr("Key"), tr("Type"), tr("Position X"), tr("Position Y")});
    m_detailsTable->horizontalHeader()->setStretchLastSection(true);
    m_detailsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_detailsTable->verticalHeader()->setVisible(false);
    m_detailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_detailsTable->setAlternatingRowColors(true);
    m_detailsTable->setStyleSheet(
        "QTableWidget { background:#1e1e2e; color:#cdd6f4; border:none; gridline-color:#313244; }"
        "QTableWidget::item { padding:4px 8px; }"
        "QTableWidget::item:selected { background:#313244; color:#cba6f7; }"
        "QHeaderView::section { background:#313244; color:#cba6f7; padding:4px; border:none; border-bottom:1px solid #45475a; }");
    detailsLayout->addWidget(m_detailsTable);

    m_tabWidget->addTab(detailsWidget, tr("📊 Details"));

    // -- Tab 2: JSON Editor ---------------------------------------------------
    auto *editorWidget = new QWidget();
    auto *editorLayout = new QVBoxLayout(editorWidget);
    editorLayout->setContentsMargins(8, 8, 8, 8);
    editorLayout->setSpacing(6);

    auto *editorHint = new QLabel(tr("Edit JSON directly. Click \"Save\" to write to disk."));
    editorHint->setStyleSheet("color:#6c7086; font-size:10px;");
    editorLayout->addWidget(editorHint);

    m_jsonEditor = new QTextEdit();
    QFont mono("Courier New", 10);
    mono.setFixedPitch(true);
    m_jsonEditor->setFont(mono);
    m_jsonEditor->setStyleSheet(
        "QTextEdit { background:#181825; color:#cdd6f4; border:1px solid #45475a; border-radius:4px; }");
    editorLayout->addWidget(m_jsonEditor);

    auto *editorBtns = new QHBoxLayout();
    m_saveEditorBtn = new QPushButton(tr("💾 Save to Disk"));
    m_saveEditorBtn->setFixedHeight(28);
    m_saveEditorBtn->setStyleSheet(
        "QPushButton{background:#313244;color:#a6e3a1;border:1px solid #45475a;border-radius:3px;}"
        "QPushButton:hover{background:#45475a;}");
    editorBtns->addStretch();
    editorBtns->addWidget(m_saveEditorBtn);
    editorLayout->addLayout(editorBtns);

    m_tabWidget->addTab(editorWidget, tr("✏️ JSON Editor"));
    splitter->addWidget(m_tabWidget);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter, 1);

    // ── Bottom apply bar ─────────────────────────────────────────────────────
    auto *bottomBar = new QHBoxLayout();
    bottomBar->setSpacing(8);

    m_applyBtn = new QPushButton(tr("✅  Apply to Device"));
    m_applyBtn->setFixedHeight(34);
    m_applyBtn->setEnabled(false);
    m_applyBtn->setStyleSheet(
        "QPushButton{background:#1e66f5;color:#fff;border:none;border-radius:4px;font-weight:bold;font-size:13px;padding:0 18px;}"
        "QPushButton:hover{background:#2575ff;}"
        "QPushButton:disabled{background:#313244;color:#585b70;}");

    auto *closeBtn = new QPushButton(tr("Close"));
    closeBtn->setFixedHeight(34);
    closeBtn->setStyleSheet(
        "QPushButton{background:#313244;color:#cdd6f4;border:1px solid #45475a;border-radius:4px;padding:0 14px;}"
        "QPushButton:hover{background:#45475a;}");

    bottomBar->addStretch();
    bottomBar->addWidget(m_applyBtn);
    bottomBar->addWidget(closeBtn);
    mainLayout->addLayout(bottomBar);

    // ── Dark background ───────────────────────────────────────────────────────
    setStyleSheet("QDialog { background:#181825; } QLabel { color:#cdd6f4; }");

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_fileList,     &QListWidget::itemClicked,      this, &KeymapDialog::onKeymapSelected);
    connect(m_applyBtn,     &QPushButton::clicked,          this, &KeymapDialog::onApplyClicked);
    connect(m_newBtn,       &QPushButton::clicked,          this, &KeymapDialog::onNewClicked);
    connect(m_deleteBtn,    &QPushButton::clicked,          this, &KeymapDialog::onDeleteClicked);
    connect(m_saveEditorBtn,&QPushButton::clicked,          this, &KeymapDialog::onSaveEditorClicked);
    connect(closeBtn,       &QPushButton::clicked,          this, &QDialog::accept);
}

// ---------------------------------------------------------------------------
// Populate file list
// ---------------------------------------------------------------------------
void KeymapDialog::refreshFileList()
{
    m_fileList->clear();
    QSet<QString> seen;

    for (const QString &dir : {getUserKeymapPath(), getDefaultKeymapPath()}) {
        const QDir d(dir);
        if (!d.exists()) continue;
        for (const QFileInfo &fi : d.entryInfoList({"*.json"}, QDir::Files | QDir::NoSymLinks)) {
            if (!seen.contains(fi.fileName())) {
                seen.insert(fi.fileName());
                auto *item = new QListWidgetItem(fi.completeBaseName());
                item->setData(Qt::UserRole, fi.fileName());
                item->setToolTip(fi.absoluteFilePath());
                m_fileList->addItem(item);
            }
        }
    }

    if (m_fileList->count() == 0) {
        m_statusLabel->setText(tr("No keymap files found."));
    } else {
        m_statusLabel->setText(tr("%1 keymap file(s) found.").arg(m_fileList->count()));
    }
}

// ---------------------------------------------------------------------------
// Select a keymap file
// ---------------------------------------------------------------------------
void KeymapDialog::onKeymapSelected(QListWidgetItem *item)
{
    if (!item) return;
    const QString fileName = item->data(Qt::UserRole).toString();
    m_currentFile = resolveKeymapPath(fileName);

    loadKeymapDetails(m_currentFile);
    loadKeymapToEditor(m_currentFile);
    m_applyBtn->setEnabled(true);
    m_statusLabel->setText(tr("Selected: %1").arg(fileName));
}

// ---------------------------------------------------------------------------
// Load keymap details into table
// ---------------------------------------------------------------------------
void KeymapDialog::loadKeymapDetails(const QString &filePath)
{
    m_detailsTable->setRowCount(0);
    m_switchKeyLabel->setText(tr("Switch Key: —"));
    m_filePathLabel->setText(filePath);

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        m_statusLabel->setText(tr("⚠️ JSON parse error: %1").arg(err.errorString()));
        return;
    }

    QJsonObject root = doc.object();

    // Switch key
    m_switchKeyLabel->setText(tr("Switch Key: <b>%1</b>").arg(root["switchKey"].toString("—")));

    // keyMapNodes
    QJsonArray nodes = root["keyMapNodes"].toArray();
    m_detailsTable->setRowCount(nodes.size());

    for (int i = 0; i < nodes.size(); ++i) {
        QJsonObject node = nodes[i].toObject();
        QString type = node["type"].toString();
        QString key  = node["key"].toString();

        // For steer wheel, show arrow keys
        if (type == "KMT_STEER_WHEEL") {
            key = QString("↑%1 ↓%2 ←%3 →%4")
                      .arg(node["upKey"].toString())
                      .arg(node["downKey"].toString())
                      .arg(node["leftKey"].toString())
                      .arg(node["rightKey"].toString());
        }

        QJsonObject pos = node.contains("pos")
                              ? node["pos"].toObject()
                              : node["centerPos"].toObject();

        m_detailsTable->setItem(i, 0, new QTableWidgetItem(key));
        m_detailsTable->setItem(i, 1, new QTableWidgetItem(keyTypeToString(type)));
        m_detailsTable->setItem(i, 2, new QTableWidgetItem(
            pos.contains("x") ? QString::number(pos["x"].toDouble(), 'f', 4) : "—"));
        m_detailsTable->setItem(i, 3, new QTableWidgetItem(
            pos.contains("y") ? QString::number(pos["y"].toDouble(), 'f', 4) : "—"));
    }
}

// ---------------------------------------------------------------------------
// Load raw JSON into editor
// ---------------------------------------------------------------------------
void KeymapDialog::loadKeymapToEditor(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    m_jsonEditor->setPlainText(doc.toJson(QJsonDocument::Indented));
}

// ---------------------------------------------------------------------------
// Save editor content back to the user keymap folder
// ---------------------------------------------------------------------------
bool KeymapDialog::saveKeymapFromEditor()
{
    if (m_currentFile.isEmpty()) {
        m_statusLabel->setText(tr("⚠️ No file selected."));
        return false;
    }

    // Validate JSON
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(m_jsonEditor->toPlainText().toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, tr("JSON Error"),
            tr("Invalid JSON: %1").arg(err.errorString()));
        return false;
    }

    // Always save to user path (never overwrite default)
    QFileInfo fi(m_currentFile);
    QString savePath = getUserKeymapPath() + "/" + fi.fileName();

    QFile out(savePath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("Save Error"),
            tr("Cannot write to:\n%1").arg(savePath));
        return false;
    }
    out.write(doc.toJson(QJsonDocument::Indented));
    out.close();

    m_currentFile = savePath;
    m_statusLabel->setText(tr("✅ Saved to %1").arg(savePath));
    return true;
}

// ---------------------------------------------------------------------------
// Apply keymap to connected device
// ---------------------------------------------------------------------------
void KeymapDialog::applyKeymap(const QString &filePath)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        m_statusLabel->setText(tr("⚠️ Device not connected: %1").arg(m_serial));
        return;
    }

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        m_statusLabel->setText(tr("⚠️ Cannot read file: %1").arg(filePath));
        return;
    }
    QString json = f.readAll();
    f.close();

    device->updateScript(json);
    m_statusLabel->setText(tr("✅ Keymap applied: %1").arg(QFileInfo(filePath).fileName()));
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void KeymapDialog::onApplyClicked()
{
    if (!m_currentFile.isEmpty())
        applyKeymap(m_currentFile);
}

void KeymapDialog::onSaveEditorClicked()
{
    saveKeymapFromEditor();
    // Reload details panel to reflect changes
    if (!m_currentFile.isEmpty())
        loadKeymapDetails(m_currentFile);
}

void KeymapDialog::onNewClicked()
{
    bool ok = false;
    QString name = QInputDialog::getText(this,
        tr("New Keymap"),
        tr("Enter a name for the new keymap (without .json):"),
        QLineEdit::Normal, tr("MyKeymap"), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    name = name.trimmed().replace(QRegularExpression("[/\\\\:*?\"<>|]"), "_");
    QString savePath = getUserKeymapPath() + "/" + name + ".json";

    if (QFile::exists(savePath)) {
        QMessageBox::warning(this, tr("Already exists"),
            tr("A keymap named \"%1\" already exists.").arg(name));
        return;
    }

    // Minimal valid template
    QJsonObject root;
    root["switchKey"] = "Key_Tab";
    root["mouseMoveMap"] = QJsonObject{
        {"type",        "KMT_MOUSE_MOVE"},
        {"comment",     ""},
        {"startPos",    QJsonObject{{"x", 0.5}, {"y", 0.5}}},
        {"speedRatioX", 1},
        {"speedRatioY", 1}
    };
    root["keyMapNodes"] = QJsonArray();
    root["width"]  = 800;
    root["height"] = 600;

    QFile out(savePath);
    if (!out.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot create file:\n%1").arg(savePath));
        return;
    }
    out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    out.close();

    refreshFileList();

    // Auto-select new file
    for (int i = 0; i < m_fileList->count(); ++i) {
        if (m_fileList->item(i)->data(Qt::UserRole).toString() == name + ".json") {
            m_fileList->setCurrentRow(i);
            onKeymapSelected(m_fileList->item(i));
            break;
        }
    }
    // Switch to editor so user can start editing immediately
    m_tabWidget->setCurrentIndex(1);
    m_statusLabel->setText(tr("✅ Created: %1").arg(name + ".json"));
}

void KeymapDialog::onDeleteClicked()
{
    auto *item = m_fileList->currentItem();
    if (!item) return;

    const QString fileName = item->data(Qt::UserRole).toString();
    const QString userPath = getUserKeymapPath() + "/" + fileName;

    if (!QFile::exists(userPath)) {
        QMessageBox::information(this, tr("Read-only"),
            tr("This is a built-in default keymap and cannot be deleted.\n"
               "Copy it to your user directory first by editing and saving it."));
        return;
    }

    if (QMessageBox::question(this, tr("Delete Keymap"),
            tr("Delete \"%1\" permanently?").arg(fileName),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QFile::remove(userPath);
    m_currentFile.clear();
    m_applyBtn->setEnabled(false);
    m_detailsTable->setRowCount(0);
    m_jsonEditor->clear();
    m_switchKeyLabel->setText(tr("Switch Key: —"));
    m_filePathLabel->clear();
    refreshFileList();
    m_statusLabel->setText(tr("🗑 Deleted: %1").arg(fileName));
}
