#include "overlaypanel.h"
#include "videoform.h"
#include "toolform.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QStandardPaths>
#include <QScrollBar>
#include <QGroupBox>
#include <QFormLayout>
#include <QFrame>

#include "QtScrcpyCore/include/QtScrcpyCore.h"

// ---------------------------------------------------------------------------
// Helper: styled section label
// ---------------------------------------------------------------------------
static QLabel *sectionLabel(const QString &text, QWidget *parent = nullptr) {
    auto *lbl = new QLabel(text, parent);
    lbl->setStyleSheet(
        "color: #94a3b8;"
        "font-size: 9px;"
        "font-weight: bold;"
        "letter-spacing: 1px;"
        "padding: 4px 0 2px 4px;"
    );
    return lbl;
}

// ---------------------------------------------------------------------------
// Helper: create a styled add-button
// ---------------------------------------------------------------------------
static QPushButton *addBtn(const QString &text, const QString &color, QWidget *parent = nullptr) {
    auto *btn = new QPushButton(text, parent);
    btn->setFixedHeight(30);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background: %1;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 5px;"
        "  font-size: 10px;"
        "  font-weight: bold;"
        "  padding: 0 6px;"
        "}"
        "QPushButton:hover {"
        "  background: %2;"
        "}"
        "QPushButton:pressed {"
        "  background: %3;"
        "}"
    ).arg(color)
     .arg(QColor(color).lighter(120).name())
     .arg(QColor(color).darker(115).name()));
    return btn;
}

// ============================================================================
//  CONSTRUCTOR
// ============================================================================
OverlayPanel::OverlayPanel(const QString &serial, QWidget *parent, QWidget *refWidget)
    : QWidget(parent), m_refWidget(refWidget), m_serial(serial)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    buildSidePanel();
    m_sidePanel->hide();
    m_currentProfileName = "custom_keymap";
    refreshProfileList();
}

OverlayPanel::~OverlayPanel() {}

// ============================================================================
//  BUILD SIDE PANEL (Main entry)
// ============================================================================
void OverlayPanel::buildSidePanel()
{
    QWidget *attachWidget = m_refWidget;
    VideoForm *vf = qobject_cast<VideoForm *>(m_refWidget);
    if (vf && vf->toolForm() && vf->toolForm()->isVisible()) {
        attachWidget = vf->toolForm();
    }
    m_sidePanel = new MagneticWidget(attachWidget, MagneticWidget::AP_OUTSIDE_RIGHT);
    m_sidePanel->setFixedWidth(280);
    QFrame *innerFrame = new QFrame(m_sidePanel);
    innerFrame->setObjectName("sidePanel");
    auto *outerLayout = new QVBoxLayout(m_sidePanel);
    outerLayout->setContentsMargins(0,0,0,0);
    outerLayout->addWidget(innerFrame);
    innerFrame->setStyleSheet(
        "QFrame#sidePanel {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        "    stop:0 #0f172a, stop:1 #1e293b);"
        "  border-left: 1px solid #334155;"
        "  border-radius: 0px;"
        "}"
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: #1e293b; width: 6px; border-radius: 3px; }"
        "QScrollBar::handle:vertical { background: #475569; border-radius: 3px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QTabWidget::pane { border: none; background: transparent; }"
        "QTabBar::tab {"
        "  background: #1e293b;"
        "  color: #94a3b8;"
        "  padding: 5px 8px;"
        "  font-size: 9px;"
        "  font-weight: bold;"
        "  border: none;"
        "  border-bottom: 2px solid transparent;"
        "}"
        "QTabBar::tab:selected { color: #38bdf8; border-bottom: 2px solid #38bdf8; background: #0f172a; }"
        "QTabBar::tab:hover { color: #e2e8f0; }"
        "QGroupBox {"
        "  color: #94a3b8;"
        "  font-size: 9px;"
        "  font-weight: bold;"
        "  border: 1px solid #334155;"
        "  border-radius: 6px;"
        "  margin-top: 8px;"
        "  padding-top: 6px;"
        "}"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 4px; left: 8px; }"
        "QLineEdit {"
        "  background: #1e293b;"
        "  color: #e2e8f0;"
        "  border: 1px solid #334155;"
        "  border-radius: 4px;"
        "  padding: 3px 6px;"
        "  font-size: 10px;"
        "}"
        "QLineEdit:focus { border: 1px solid #38bdf8; }"
        "QComboBox {"
        "  background: #1e293b;"
        "  color: #e2e8f0;"
        "  border: 1px solid #334155;"
        "  border-radius: 4px;"
        "  padding: 3px 6px;"
        "  font-size: 10px;"
        "}"
        "QComboBox::drop-down { border: none; width: 16px; }"
        "QSlider::groove:horizontal { background: #334155; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #38bdf8; width: 14px; height: 14px; border-radius: 7px; margin: -5px 0; }"
        "QSlider::sub-page:horizontal { background: #38bdf8; border-radius: 2px; }"
        "QLabel { color: #cbd5e1; font-size: 10px; }"
        "QCheckBox { color: #cbd5e1; font-size: 10px; }"
        "QSpinBox {"
        "  background: #1e293b; color: #e2e8f0;"
        "  border: 1px solid #334155; border-radius: 4px; padding: 2px 4px;"
        "  font-size: 10px;"
        "}"
        "QListWidget {"
        "  background: #1e293b; color: #e2e8f0;"
        "  border: 1px solid #334155; border-radius: 4px;"
        "  font-size: 10px;"
        "}"
        "QListWidget::item:selected { background: #2563eb; }"
    );

    auto *mainLayout = new QVBoxLayout(innerFrame);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- Header ---
    auto *header = new QWidget(innerFrame);
    header->setFixedHeight(44);
    header->setStyleSheet("background: #020b18; border-bottom: 1px solid #334155;");
    auto *headerL = new QHBoxLayout(header);
    headerL->setContentsMargins(10, 0, 10, 0);

    auto *titleLbl = new QLabel("KEYMAP STUDIO", header);
    titleLbl->setStyleSheet("color: #38bdf8; font-size: 11px; font-weight: bold; letter-spacing: 2px;");

    auto *proBadge = new QLabel("PRO", header);
    proBadge->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #2563eb,stop:1 #7c3aed);"
        "color: white; font-size: 8px; font-weight: bold; padding: 2px 6px; border-radius: 3px;"
    );

    headerL->addWidget(titleLbl);
    headerL->addStretch();
    headerL->addWidget(proBadge);
    mainLayout->addWidget(header);

    // --- Profile selector ---
    auto *profileBar = new QWidget(innerFrame);
    profileBar->setStyleSheet("background: #0f172a; border-bottom: 1px solid #1e293b; padding: 6px;");
    auto *profileL = new QHBoxLayout(profileBar);
    profileL->setContentsMargins(8, 4, 8, 4);
    profileL->setSpacing(6);

    m_presetCombo = new QComboBox(profileBar);
    m_presetCombo->setFixedHeight(28);
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OverlayPanel::onProfilePresetSelected);

    m_profileEdit = new QLineEdit(profileBar);
    m_profileEdit->setFixedHeight(28);
    m_profileEdit->setPlaceholderText("Profile name...");
    m_profileEdit->setText(m_currentProfileName);

    profileL->addWidget(m_presetCombo, 2);
    profileL->addWidget(m_profileEdit, 2);
    mainLayout->addWidget(profileBar);

    // --- Tabbed content ---
    m_tabs = new QTabWidget(innerFrame);
    m_tabs->setDocumentMode(true);

    // Tab 1: Basic Controls
    auto *basicTab = new QWidget;
    auto *basicScroll = new QScrollArea;
    basicScroll->setWidget(basicTab);
    basicScroll->setWidgetResizable(true);
    buildBasicTab(basicTab);
    m_tabs->addTab(basicScroll, "Basic");

    // Tab 2: Game Controls
    auto *gameTab = new QWidget;
    auto *gameScroll = new QScrollArea;
    gameScroll->setWidget(gameTab);
    gameScroll->setWidgetResizable(true);
    buildGameTab(gameTab);
    m_tabs->addTab(gameScroll, "Game");

    // Tab 3: Advanced (Macro/Spray)
    auto *advTab = new QWidget;
    auto *advScroll = new QScrollArea;
    advScroll->setWidget(advTab);
    advScroll->setWidgetResizable(true);
    buildAdvancedTab(advTab);
    m_tabs->addTab(advScroll, "Advanced");

    // Tab 4: Properties Inspector (dynamic)
    auto *propsTab = new QWidget;
    auto *propsScroll = new QScrollArea;
    propsScroll->setWidget(propsTab);
    propsScroll->setWidgetResizable(true);
    auto *propsL = new QVBoxLayout(propsTab);
    propsL->setContentsMargins(8, 8, 8, 8);
    propsL->setSpacing(6);
    buildPropsPanel(propsL);
    propsL->addStretch();
    m_tabs->addTab(propsScroll, "Props");

    // Tab 5: Settings
    auto *settingsTab = new QWidget;
    auto *settingsScroll = new QScrollArea;
    settingsScroll->setWidget(settingsTab);
    settingsScroll->setWidgetResizable(true);
    buildSettingsTab(settingsTab);
    m_tabs->addTab(settingsScroll, "Settings");

    mainLayout->addWidget(m_tabs, 1);

    // --- Bottom action bar ---
    auto *actionBar = new QWidget(innerFrame);
    actionBar->setFixedHeight(88);
    actionBar->setStyleSheet("background: #020b18; border-top: 1px solid #334155; padding: 6px;");
    auto *actionL = new QVBoxLayout(actionBar);
    actionL->setContentsMargins(8, 6, 8, 6);
    actionL->setSpacing(4);

    m_saveBtn = addBtn("Save & Apply", "#16a34a");
    connect(m_saveBtn, &QPushButton::clicked, this, &OverlayPanel::onSaveAndApply);
    actionL->addWidget(m_saveBtn);

    auto *row2 = new QHBoxLayout;
    row2->setSpacing(4);

    m_hudToggleBtn = addBtn("HUD: ON", "#475569");
    connect(m_hudToggleBtn, &QPushButton::clicked, this, &OverlayPanel::onToggleHUD);
    row2->addWidget(m_hudToggleBtn);

    m_closeBtn = addBtn("Close", "#64748b");
    connect(m_closeBtn, &QPushButton::clicked, this, &OverlayPanel::onCloseEdit);
    row2->addWidget(m_closeBtn);
    actionL->addLayout(row2);
    mainLayout->addWidget(actionBar);
}

// ============================================================================
//  BASIC TAB - Click, R-Click, WASD, Aim, Swipe, FreeLook
// ============================================================================
void OverlayPanel::buildBasicTab(QWidget *tab)
{
    auto *l = new QVBoxLayout(tab);
    l->setContentsMargins(8, 8, 8, 8);
    l->setSpacing(6);

    // --- Mouse Click section ---
    auto *clickGroup = new QGroupBox("Mouse Clicks", tab);
    auto *clickL = new QVBoxLayout(clickGroup);
    clickL->setSpacing(4);

    auto *r1 = new QHBoxLayout;
    auto *lClickBtn = addBtn("L-Click (Fire)", OverlayButton::typeColor(OverlayButtonType::Click).name());
    auto *rClickBtn = addBtn("R-Click (Scope)", OverlayButton::typeColor(OverlayButtonType::RightClick).name());
    connect(lClickBtn, &QPushButton::clicked, this, &OverlayPanel::onAddClick);
    connect(rClickBtn, &QPushButton::clicked, this, &OverlayPanel::onAddRightClick);
    r1->addWidget(lClickBtn);
    r1->addWidget(rClickBtn);
    clickL->addLayout(r1);

    auto *r2 = new QHBoxLayout;
    auto *dblClickBtn = addBtn("Double Tap", OverlayButton::typeColor(OverlayButtonType::DoubleClick).name());
    auto *midClickBtn = addBtn("Mid Click", OverlayButton::typeColor(OverlayButtonType::MiddleClick).name());
    connect(dblClickBtn, &QPushButton::clicked, this, &OverlayPanel::onAddDoubleClick);
    connect(midClickBtn, &QPushButton::clicked, this, &OverlayPanel::onAddMiddleClick);
    r2->addWidget(dblClickBtn);
    r2->addWidget(midClickBtn);
    clickL->addLayout(r2);
    l->addWidget(clickGroup);

    // --- Movement section ---
    auto *moveGroup = new QGroupBox("Movement", tab);
    auto *moveL = new QVBoxLayout(moveGroup);
    moveL->setSpacing(4);

    auto *wasdBtn = addBtn("WASD Move", OverlayButton::typeColor(OverlayButtonType::Joystick).name());
    connect(wasdBtn, &QPushButton::clicked, this, &OverlayPanel::onAddJoystick);
    moveL->addWidget(wasdBtn);

    auto *r3 = new QHBoxLayout;
    auto *aimBtn = addBtn("Aim / Look", OverlayButton::typeColor(OverlayButtonType::Aim).name());
    auto *freeLookBtn = addBtn("Free Look", OverlayButton::typeColor(OverlayButtonType::FreeLook).name());
    connect(aimBtn, &QPushButton::clicked, this, &OverlayPanel::onAddAim);
    connect(freeLookBtn, &QPushButton::clicked, this, &OverlayPanel::onAddFreeLook);
    r3->addWidget(aimBtn);
    r3->addWidget(freeLookBtn);
    moveL->addLayout(r3);
    l->addWidget(moveGroup);

    // --- Gesture section ---
    auto *gestureGroup = new QGroupBox("Gestures", tab);
    auto *gestureL = new QVBoxLayout(gestureGroup);
    gestureL->setSpacing(4);
    auto *swipeBtn = addBtn("Swipe Gesture", OverlayButton::typeColor(OverlayButtonType::Swipe).name());
    connect(swipeBtn, &QPushButton::clicked, this, &OverlayPanel::onAddSwipe);
    gestureL->addWidget(swipeBtn);
    l->addWidget(gestureGroup);

    // --- Management ---
    auto *mgmtGroup = new QGroupBox("Layout", tab);
    auto *mgmtL = new QVBoxLayout(mgmtGroup);
    mgmtL->setSpacing(4);

    auto *r4 = new QHBoxLayout;
    m_importBtn = addBtn("Import JSON", "#7c3aed");
    m_clearAllBtn = addBtn("Clear All", "#dc2626");
    connect(m_importBtn, &QPushButton::clicked, this, &OverlayPanel::onImportKeymap);
    connect(m_clearAllBtn, &QPushButton::clicked, this, &OverlayPanel::onClearAll);
    r4->addWidget(m_importBtn);
    r4->addWidget(m_clearAllBtn);
    mgmtL->addLayout(r4);

    auto *exportBtn2 = addBtn("Export JSON", "#2563eb");
    connect(exportBtn2, &QPushButton::clicked, this, &OverlayPanel::onExportKeymap);
    mgmtL->addWidget(exportBtn2);
    l->addWidget(mgmtGroup);

    l->addStretch();
}

// ============================================================================
//  GAME TAB - FPS/Battle Royale/MOBA specific controls
// ============================================================================
void OverlayPanel::buildGameTab(QWidget *tab)
{
    auto *l = new QVBoxLayout(tab);
    l->setContentsMargins(8, 8, 8, 8);
    l->setSpacing(6);

    // --- Combat ---
    auto *combatGroup = new QGroupBox("Combat", tab);
    auto *combatL = new QVBoxLayout(combatGroup);
    combatL->setSpacing(4);

    auto *r1 = new QHBoxLayout;
    auto *fireBtn   = addBtn("Fire",   OverlayButton::typeColor(OverlayButtonType::Click).name());
    auto *scopeBtn  = addBtn("Scope",  OverlayButton::typeColor(OverlayButtonType::Scope).name());
    connect(fireBtn,  &QPushButton::clicked, this, &OverlayPanel::onAddFire);
    connect(scopeBtn, &QPushButton::clicked, this, &OverlayPanel::onAddScope);
    r1->addWidget(fireBtn);
    r1->addWidget(scopeBtn);
    combatL->addLayout(r1);

    auto *r2 = new QHBoxLayout;
    auto *grenadeBtn = addBtn("Grenade", OverlayButton::typeColor(OverlayButtonType::Grenade).name());
    auto *jumpBtn    = addBtn("Jump",    OverlayButton::typeColor(OverlayButtonType::Jump).name());
    connect(grenadeBtn, &QPushButton::clicked, this, &OverlayPanel::onAddGrenade);
    connect(jumpBtn,    &QPushButton::clicked, this, &OverlayPanel::onAddJump);
    r2->addWidget(grenadeBtn);
    r2->addWidget(jumpBtn);
    combatL->addLayout(r2);

    auto *proneBtn = addBtn("Prone / Crouch", OverlayButton::typeColor(OverlayButtonType::Prone).name());
    connect(proneBtn, &QPushButton::clicked, this, &OverlayPanel::onAddProne);
    combatL->addWidget(proneBtn);
    l->addWidget(combatGroup);

    // --- MOBA / Skills ---
    auto *skillGroup = new QGroupBox("Skills (MOBA / RPG)", tab);
    auto *skillL = new QVBoxLayout(skillGroup);
    skillL->setSpacing(4);

    for (int i = 1; i <= 4; i++) {
        auto *skBtn = addBtn(QString("Skill %1 (Q/W/E/R)").arg(i), "#9b59b6");
        connect(skBtn, &QPushButton::clicked, this, &OverlayPanel::onAddSkill);
        skillL->addWidget(skBtn);
    }
    l->addWidget(skillGroup);

    // --- Utility ---
    auto *utilGroup = new QGroupBox("Utility", tab);
    auto *utilL = new QVBoxLayout(utilGroup);
    utilL->setSpacing(4);

    auto *r3 = new QHBoxLayout;
    auto *mapBtn = addBtn("Map (M)",   OverlayButton::typeColor(OverlayButtonType::Map).name());
    auto *bagBtn = addBtn("Bag (Tab)", OverlayButton::typeColor(OverlayButtonType::Bag).name());
    connect(mapBtn, &QPushButton::clicked, this, &OverlayPanel::onAddMap);
    connect(bagBtn, &QPushButton::clicked, this, &OverlayPanel::onAddBag);
    r3->addWidget(mapBtn);
    r3->addWidget(bagBtn);
    utilL->addLayout(r3);

    auto *vehicleBtn = addBtn("Vehicle / Drive", OverlayButton::typeColor(OverlayButtonType::Vehicle).name());
    connect(vehicleBtn, &QPushButton::clicked, this, &OverlayPanel::onAddVehicle);
    utilL->addWidget(vehicleBtn);
    l->addWidget(utilGroup);

    l->addStretch();
}

// ============================================================================
//  ADVANCED TAB - Macros, Spray, Auto-fire
// ============================================================================
void OverlayPanel::buildAdvancedTab(QWidget *tab)
{
    auto *l = new QVBoxLayout(tab);
    l->setContentsMargins(8, 8, 8, 8);
    l->setSpacing(6);

    // --- Macro ---
    auto *macroGroup = new QGroupBox("Macro (Multi-key sequence)", tab);
    auto *macroL = new QVBoxLayout(macroGroup);
    macroL->setSpacing(4);

    auto *macroHint = new QLabel("Record a timed sequence of key presses.", macroGroup);
    macroHint->setWordWrap(true);
    macroHint->setStyleSheet("color: #64748b; font-size: 9px;");
    macroL->addWidget(macroHint);

    auto *addMacroBtn = addBtn("+ Add Macro Button", OverlayButton::typeColor(OverlayButtonType::Macro).name());
    connect(addMacroBtn, &QPushButton::clicked, this, &OverlayPanel::onAddMacro);
    macroL->addWidget(addMacroBtn);
    l->addWidget(macroGroup);

    // --- Auto-fire / Spray ---
    auto *sprayGroup = new QGroupBox("Auto-Fire / Spray", tab);
    auto *sprayL = new QVBoxLayout(sprayGroup);
    sprayL->setSpacing(4);

    auto *sprayHint = new QLabel("Fires automatically while the key is held.", sprayGroup);
    sprayHint->setWordWrap(true);
    sprayHint->setStyleSheet("color: #64748b; font-size: 9px;");
    sprayL->addWidget(sprayHint);

    auto *addSprayBtn = addBtn("+ Add Auto-Fire Button", OverlayButton::typeColor(OverlayButtonType::Spray).name());
    connect(addSprayBtn, &QPushButton::clicked, this, &OverlayPanel::onAddSpray);
    sprayL->addWidget(addSprayBtn);
    l->addWidget(sprayGroup);

    // --- WASD+ presets ---
    auto *presetGroup = new QGroupBox("WASD+ Presets", tab);
    auto *presetL = new QVBoxLayout(presetGroup);
    presetL->setSpacing(4);

    struct Preset { QString name; QString color; };
    QList<Preset> presets = {
        {"FPS Full Setup",      "#e74c3c"},
        {"Battle Royale Setup", "#e67e22"},
        {"MOBA Setup",          "#9b59b6"},
        {"Racing / Vehicle",    "#3498db"},
    };

    for (const auto &preset : presets) {
        auto *pb = addBtn(preset.name, preset.color);
        presetL->addWidget(pb);
    }
    l->addWidget(presetGroup);

    l->addStretch();
}

// ============================================================================
//  PROPERTIES PANEL
// ============================================================================
void OverlayPanel::buildPropsPanel(QVBoxLayout *l)
{
    m_propsWidget = new QWidget;
    auto *propsL = new QVBoxLayout(m_propsWidget);
    propsL->setContentsMargins(0, 0, 0, 0);
    propsL->setSpacing(6);

    // Status label
    m_selTypeLabel = new QLabel("No button selected", m_propsWidget);
    m_selTypeLabel->setStyleSheet(
        "color: #64748b; font-style: italic; font-size: 10px; padding: 8px 0;"
    );
    m_selTypeLabel->setAlignment(Qt::AlignCenter);
    propsL->addWidget(m_selTypeLabel);

    // --- Label & Key ---
    auto *basicGroup = new QGroupBox("Button", m_propsWidget);
    auto *basicL = new QFormLayout(basicGroup);
    basicL->setSpacing(4);
    basicL->setContentsMargins(8, 8, 8, 8);

    m_labelEdit = new QLineEdit(m_propsWidget);
    m_labelEdit->setPlaceholderText("Button label...");
    basicL->addRow("Label:", m_labelEdit);

    auto *keyRow = new QHBoxLayout;
    m_keyEdit = new QLineEdit(m_propsWidget);
    m_keyEdit->setPlaceholderText("Key_*");
    m_keyEdit->setReadOnly(true);
    m_recordKeyBtn = new QPushButton("Bind Key", m_propsWidget);
    m_recordKeyBtn->setFixedSize(70, 24);
    m_recordKeyBtn->setStyleSheet(
        "background: #2563eb; color: white; border-radius: 4px; font-size: 9px; font-weight: bold;"
    );
    connect(m_recordKeyBtn, &QPushButton::clicked, this, &OverlayPanel::onRecordKeyClicked);
    keyRow->addWidget(m_keyEdit);
    keyRow->addWidget(m_recordKeyBtn);
    basicL->addRow("Key:", keyRow);

    auto *sizeRow = new QHBoxLayout;
    m_sizeSlider = new QSlider(Qt::Horizontal, m_propsWidget);
    m_sizeSlider->setRange(20, 150);
    m_sizeSlider->setValue(55);
    sizeRow->addWidget(m_sizeSlider);
    basicL->addRow("Size:", sizeRow);

    m_coordsLabel = new QLabel("X: -- | Y: --", m_propsWidget);
    m_coordsLabel->setStyleSheet("color: #64748b; font-size: 9px;");
    basicL->addRow("Pos:", m_coordsLabel);

    propsL->addWidget(basicGroup);

    // --- Joystick group ---
    m_joyGroup = new QGroupBox("WASD Keys", m_propsWidget);
    auto *joyL = new QFormLayout(m_joyGroup);
    joyL->setSpacing(4);
    joyL->setContentsMargins(8, 8, 8, 8);
    m_joyUpEdit    = new QLineEdit("Key_W", m_joyGroup);
    m_joyDownEdit  = new QLineEdit("Key_S", m_joyGroup);
    m_joyLeftEdit  = new QLineEdit("Key_A", m_joyGroup);
    m_joyRightEdit = new QLineEdit("Key_D", m_joyGroup);
    joyL->addRow("Up:",    m_joyUpEdit);
    joyL->addRow("Down:",  m_joyDownEdit);
    joyL->addRow("Left:",  m_joyLeftEdit);
    joyL->addRow("Right:", m_joyRightEdit);
    propsL->addWidget(m_joyGroup);

    // --- Aim group ---
    m_aimGroup = new QGroupBox("Aim Sensitivity", m_propsWidget);
    auto *aimL = new QFormLayout(m_aimGroup);
    aimL->setSpacing(4);
    aimL->setContentsMargins(8, 8, 8, 8);
    m_speedXSlider = new QSlider(Qt::Horizontal, m_aimGroup);
    m_speedXSlider->setRange(5, 100);
    m_speedXSlider->setValue(25);
    m_speedYSlider = new QSlider(Qt::Horizontal, m_aimGroup);
    m_speedYSlider->setRange(5, 100);
    m_speedYSlider->setValue(25);
    m_speedValLabel = new QLabel("X: 2.5  Y: 2.5", m_aimGroup);
    m_speedValLabel->setStyleSheet("color: #64748b; font-size: 9px;");
    aimL->addRow("Speed X:", m_speedXSlider);
    aimL->addRow("Speed Y:", m_speedYSlider);
    aimL->addRow("", m_speedValLabel);
    propsL->addWidget(m_aimGroup);

    // --- Click group ---
    m_clickGroup = new QGroupBox("Click Options", m_propsWidget);
    auto *clickL2 = new QVBoxLayout(m_clickGroup);
    clickL2->setContentsMargins(8, 8, 8, 8);
    m_switchMapCheck = new QCheckBox("Hold to aim (switchMap)", m_clickGroup);
    clickL2->addWidget(m_switchMapCheck);
    propsL->addWidget(m_clickGroup);

    // --- Spray group ---
    m_sprayGroup = new QGroupBox("Auto-Fire Settings", m_propsWidget);
    auto *sprayL2 = new QFormLayout(m_sprayGroup);
    sprayL2->setContentsMargins(8, 8, 8, 8);
    m_sprayInterval = new QSpinBox(m_sprayGroup);
    m_sprayInterval->setRange(10, 500);
    m_sprayInterval->setValue(80);
    m_sprayInterval->setSuffix(" ms");
    sprayL2->addRow("Interval:", m_sprayInterval);
    propsL->addWidget(m_sprayGroup);

    // --- Swipe group ---
    m_swipeGroup = new QGroupBox("Swipe End Position", m_propsWidget);
    auto *swipeL2 = new QFormLayout(m_swipeGroup);
    swipeL2->setContentsMargins(8, 8, 8, 8);
    m_swipeEndXEdit = new QLineEdit("0.50", m_swipeGroup);
    m_swipeEndYEdit = new QLineEdit("0.30", m_swipeGroup);
    swipeL2->addRow("End X (0-1):", m_swipeEndXEdit);
    swipeL2->addRow("End Y (0-1):", m_swipeEndYEdit);
    propsL->addWidget(m_swipeGroup);

    // --- Macro group ---
    m_macroGroup = new QGroupBox("Macro Steps", m_propsWidget);
    auto *macroL2 = new QVBoxLayout(m_macroGroup);
    macroL2->setContentsMargins(8, 8, 8, 8);
    macroL2->setSpacing(4);

    m_macroList = new QListWidget(m_macroGroup);
    m_macroList->setFixedHeight(80);
    macroL2->addWidget(m_macroList);

    auto *stepRow = new QHBoxLayout;
    m_macroKeyEdit = new QLineEdit(m_macroGroup);
    m_macroKeyEdit->setPlaceholderText("Key_X");
    m_macroKeyEdit->setFixedWidth(70);
    m_macroDelay = new QSpinBox(m_macroGroup);
    m_macroDelay->setRange(10, 2000);
    m_macroDelay->setValue(50);
    m_macroDelay->setSuffix("ms");
    m_macroDelay->setFixedWidth(65);
    auto *addStepBtn = addBtn("+", "#16a34a");
    addStepBtn->setFixedSize(28, 24);
    connect(addStepBtn, &QPushButton::clicked, this, &OverlayPanel::onMacroStepAdd);
    auto *delStepBtn = addBtn("-", "#dc2626");
    delStepBtn->setFixedSize(28, 24);
    connect(delStepBtn, &QPushButton::clicked, this, &OverlayPanel::onMacroStepRemove);
    stepRow->addWidget(m_macroKeyEdit);
    stepRow->addWidget(m_macroDelay);
    stepRow->addWidget(addStepBtn);
    stepRow->addWidget(delStepBtn);
    macroL2->addLayout(stepRow);
    propsL->addWidget(m_macroGroup);

    // --- Action buttons ---
    auto *actRow = new QHBoxLayout;
    auto *applyBtn = addBtn("Apply", "#16a34a");
    connect(applyBtn, &QPushButton::clicked, this, &OverlayPanel::onApplyProps);
    m_deleteBtn = addBtn("Delete", "#dc2626");
    connect(m_deleteBtn, &QPushButton::clicked, this, &OverlayPanel::onDeleteSelected);
    actRow->addWidget(applyBtn);
    actRow->addWidget(m_deleteBtn);
    propsL->addLayout(actRow);

    // disable by default
    m_propsWidget->setEnabled(false);
    l->addWidget(m_propsWidget);
}

// ============================================================================
//  SETTINGS TAB
// ============================================================================
void OverlayPanel::buildSettingsTab(QWidget *tab)
{
    auto *l = new QVBoxLayout(tab);
    l->setContentsMargins(8, 8, 8, 8);
    l->setSpacing(8);

    auto *hudGroup = new QGroupBox("HUD Settings", tab);
    auto *hudL = new QFormLayout(hudGroup);
    hudL->setContentsMargins(8, 8, 8, 8);

    m_opacitySlider = new QSlider(Qt::Horizontal, hudGroup);
    m_opacitySlider->setRange(10, 100);
    m_opacitySlider->setValue(85);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &OverlayPanel::onOpacitySliderChanged);
    hudL->addRow("Opacity:", m_opacitySlider);
    l->addWidget(hudGroup);

    auto *keyGroup = new QGroupBox("Keymap Settings", tab);
    auto *keyL = new QFormLayout(keyGroup);
    keyL->setContentsMargins(8, 8, 8, 8);
    auto *switchKeyEdit = new QLineEdit("Key_QuoteLeft", keyGroup);
    keyL->addRow("Switch Key:", switchKeyEdit);
    l->addWidget(keyGroup);

    auto *resetBtn = addBtn("Reset Standard WASD", "#dc2626");
    l->addWidget(resetBtn);

    auto *hintLbl = new QLabel(
        "Tips:\n"
        "- Double-click on video to add button\n"
        "- Drag buttons to reposition\n"
        "- Right-click a button for options\n"
        "- Press ESC to cancel key binding\n"
        "- Save & Apply to activate",
        tab
    );
    hintLbl->setWordWrap(true);
    hintLbl->setStyleSheet("color: #475569; font-size: 9px; padding: 8px;");
    l->addWidget(hintLbl);
    l->addStretch();
}
// ============================================================================
//  setEditMode / setOverlayVisible / resize / paint
// ============================================================================
void OverlayPanel::setEditMode(bool edit)
{
    m_editMode = edit;
    for (auto *b : m_buttons) b->setEditMode(edit);

    if (edit) {
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        
        // Initial placement next to the video form (magnetic widget will handle snap)
        if (m_refWidget && m_sidePanel) {
            QWidget* pw = m_refWidget;
            m_sidePanel->move(pw->pos().x() + pw->width(), pw->pos().y() + 30);
        }
        
        m_sidePanel->show();
        updateSidePanelGeometry();
        setFocus();
    } else {
        setAttribute(Qt::WA_TransparentForMouseEvents, !m_overlayOn);
        m_sidePanel->hide();
    }
    update();
}

void OverlayPanel::setOverlayVisible(bool v)
{
    m_overlayOn = v;
    if (!m_editMode) setAttribute(Qt::WA_TransparentForMouseEvents, !v);
    for (auto *b : m_buttons) {
        b->setVisible(v || m_editMode);
    }
    if (m_hudToggleBtn) {
        m_hudToggleBtn->setText(v ? "HUD: ON" : "HUD: OFF");
        m_hudToggleBtn->setStyleSheet(v
            ? "background:#16a34a;color:white;border-radius:5px;font-size:10px;font-weight:bold;"
            : "background:#475569;color:white;border-radius:5px;font-size:10px;font-weight:bold;"
        );
    }
    update();
}

void OverlayPanel::onParentResized()
{
    resize(m_refWidget->size());
    updateSidePanelGeometry();
    QRect area = currentVideoGeometry();
    for (auto *b : m_buttons) b->reposition(area);
}

void OverlayPanel::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    updateSidePanelGeometry();
}

void OverlayPanel::updateSidePanelGeometry()
{
    if (!m_sidePanel || !m_refWidget) return;
    
    // Auto-resize the side panel height to match the main window
    int h = m_refWidget->height();
    if (m_sidePanel->height() != h) {
        m_sidePanel->resize(m_sidePanel->width(), h);
    }
}

QRect OverlayPanel::currentVideoGeometry() const
{
    // The panel is now external, so the video area is just the full rect
    if (m_refWidget) {
        return QRect(0, 0, width(), height());
    }
    return rect();
}

// ============================================================================
//  PAINT - Grid overlay + toast
// ============================================================================
void OverlayPanel::paintEvent(QPaintEvent *)
{
    if (!m_editMode && m_toastMessage.isEmpty()) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (m_editMode) {
        // Dark vignette on video area
        QRect vr = currentVideoGeometry();
        p.fillRect(vr, QColor(0, 0, 0, 40));

        // Grid
        p.setPen(QPen(QColor(59, 130, 246, 25), 1));
        int gx = 30, gy = 30;
        for (int x = vr.left(); x < vr.right(); x += gx)
            p.drawLine(x, vr.top(), x, vr.bottom());
        for (int y = vr.top(); y < vr.bottom(); y += gy)
            p.drawLine(vr.left(), y, vr.right(), y);

        // Center crosshair
        p.setPen(QPen(QColor(59, 130, 246, 60), 1, Qt::DashLine));
        p.drawLine(vr.center().x(), vr.top(), vr.center().x(), vr.bottom());
        p.drawLine(vr.left(), vr.center().y(), vr.right(), vr.center().y());

        // Key recording indicator
        if (m_recordingKey) {
            QRect badge(vr.center().x() - 100, vr.top() + 10, 200, 28);
            p.setBrush(QColor(239, 68, 68, 220));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(badge, 5, 5);
            p.setPen(Qt::white);
            p.setFont(QFont("Segoe UI", 9, QFont::Bold));
            p.drawText(badge, Qt::AlignCenter, "Press any key to bind...");
        }
    }

    // Toast
    if (!m_toastMessage.isEmpty()) {
        QFontMetrics fm(QFont("Segoe UI", 10, QFont::Bold));
        int tw = fm.horizontalAdvance(m_toastMessage) + 30;
        int th = 36;
        QRect tr((width() - tw) / 2, height() - 60, tw, th);
        p.setBrush(QColor(15, 23, 42, 230));
        p.setPen(QPen(QColor(56, 189, 248), 1));
        p.drawRoundedRect(tr, 8, 8);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 10, QFont::Bold));
        p.drawText(tr, Qt::AlignCenter, m_toastMessage);
    }
}

// ============================================================================
//  Mouse / Key events on canvas
// ============================================================================
void OverlayPanel::mousePressEvent(QMouseEvent *e)
{
    if (!m_editMode) return;
    if (e->button() == Qt::LeftButton) {
        // Deselect if clicked on empty canvas
        bool hitBtn = false;
        for (auto *b : m_buttons) {
            if (b->geometry().contains(e->pos())) { hitBtn = true; break; }
        }
        if (!hitBtn) selectButton(nullptr);
    }
}

void OverlayPanel::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (!m_editMode) return;
    if (e->button() == Qt::LeftButton) {
        QRect vArea = currentVideoGeometry();
        if (vArea.contains(e->pos())) {
            double rx = static_cast<double>(e->pos().x() - vArea.left()) / vArea.width();
            double ry = static_cast<double>(e->pos().y() - vArea.top())  / vArea.height();
            createButton(OverlayButtonType::Click, "Fire", "Key_J", QPointF(rx, ry));
        }
    }
}

void OverlayPanel::keyPressEvent(QKeyEvent *e)
{
    if (!m_editMode) { QWidget::keyPressEvent(e); return; }
    if (m_recordingKey) {
        if (e->key() == Qt::Key_Escape) {
            m_recordingKey = false;
            if (m_recordKeyBtn) m_recordKeyBtn->setText("Bind Key");
            update(); return;
        }
        QString keyStr = qtKeyToString(e->key());
        if (!keyStr.isEmpty() && m_keyEdit) {
            m_keyEdit->setText(keyStr);
        }
        m_recordingKey = false;
        if (m_recordKeyBtn) m_recordKeyBtn->setText("Bind Key");
        update(); return;
    }
    if (e->key() == Qt::Key_Delete && m_selected) onDeleteSelected();
    QWidget::keyPressEvent(e);
}

void OverlayPanel::keyReleaseEvent(QKeyEvent *e) { QWidget::keyReleaseEvent(e); }

// ============================================================================
//  BUTTON FACTORY
// ============================================================================
OverlayButton *OverlayPanel::createButton(OverlayButtonType type,
                                          const QString &label,
                                          const QString &key,
                                          QPointF posRatio)
{
    auto *btn = new OverlayButton(type, label, key, posRatio, this);
    btn->setHudOpacity(m_hudOpacity);
    btn->setEditMode(m_editMode);
    btn->reposition(currentVideoGeometry());
    btn->show();

    connect(btn, &OverlayButton::editRequested,      this, &OverlayPanel::onButtonEditRequested);
    connect(btn, &OverlayButton::posRatioChanged,    this, &OverlayPanel::onButtonPosChanged);
    connect(btn, &OverlayButton::deleteRequested,    this, &OverlayPanel::onButtonDeleteRequested);
    connect(btn, &OverlayButton::duplicateRequested, this, &OverlayPanel::onButtonDuplicateRequested);

    m_buttons.append(btn);
    emit layoutChanged();
    return btn;
}

// ============================================================================
//  ADD SLOTS - Basic
// ============================================================================
void OverlayPanel::onAddClick()      { createButton(OverlayButtonType::Click,       "Fire",       "Key_J",     {0.15, 0.65}); }
void OverlayPanel::onAddDoubleClick(){ createButton(OverlayButtonType::DoubleClick, "Double",     "Key_H",     {0.85, 0.55}); }
void OverlayPanel::onAddRightClick() { createButton(OverlayButtonType::RightClick,  "Scope",      "Key_L",     {0.85, 0.35}); }
void OverlayPanel::onAddMiddleClick(){ createButton(OverlayButtonType::MiddleClick, "Mid",        "Key_M",     {0.50, 0.50}); }
void OverlayPanel::onAddJoystick()   { createButton(OverlayButtonType::Joystick,   "WASD",       "WASD",      {0.25, 0.70}); }
void OverlayPanel::onAddAim()        { createButton(OverlayButtonType::Aim,         "Aim",        "Key_QuoteLeft", {0.60, 0.40}); }
void OverlayPanel::onAddSwipe()      { createButton(OverlayButtonType::Swipe,       "Swipe",      "Key_G",     {0.50, 0.50}); }
void OverlayPanel::onAddFreeLook()   { createButton(OverlayButtonType::FreeLook,   "FreeLook",   "Key_F",     {0.65, 0.35}); }

// --- Compat slots ---
void OverlayPanel::onSetLeftClick()  { onAddClick(); }
void OverlayPanel::onSetRightClick() { onAddRightClick(); }
void OverlayPanel::onSetMidClick()   { onAddMiddleClick(); }

// ============================================================================
//  ADD SLOTS - Game Controls
// ============================================================================
void OverlayPanel::onAddFire()    { createButton(OverlayButtonType::Click,   "Fire",    "Key_J",     {0.15, 0.65}); }
void OverlayPanel::onAddScope()   { createButton(OverlayButtonType::Scope,   "Scope",   "Key_L",     {0.85, 0.35}); }
void OverlayPanel::onAddJump()    { createButton(OverlayButtonType::Jump,    "Jump",    "Key_Space", {0.50, 0.80}); }
void OverlayPanel::onAddProne()   { createButton(OverlayButtonType::Prone,   "Prone",   "Key_Z",     {0.80, 0.80}); }
void OverlayPanel::onAddGrenade() { createButton(OverlayButtonType::Grenade, "Grenade", "Key_G",     {0.30, 0.30}); }
void OverlayPanel::onAddMap()     { createButton(OverlayButtonType::Map,     "Map",     "Key_M",     {0.90, 0.10}); }
void OverlayPanel::onAddBag()     { createButton(OverlayButtonType::Bag,     "Bag",     "Key_Tab",   {0.90, 0.25}); }
void OverlayPanel::onAddVehicle() { createButton(OverlayButtonType::Vehicle, "Drive",   "WASD",      {0.25, 0.75}); }
void OverlayPanel::onAddSkill()   { createButton(OverlayButtonType::Skill,   "Skill",   "Key_Q",     {0.70, 0.70}); }

// ============================================================================
//  ADD SLOTS - Advanced
// ============================================================================
void OverlayPanel::onAddMacro()
{
    auto *btn = createButton(OverlayButtonType::Macro, "Macro", "Key_X", {0.50, 0.50});
    // Default macro: press, wait 50ms, release
    btn->addMacroStep({"Key_X", 50, true});
    btn->addMacroStep({"Key_X", 50, false});
}

void OverlayPanel::onAddSpray()
{
    auto *btn = createButton(OverlayButtonType::Spray, "Auto-Fire", "Key_J", {0.15, 0.65});
    btn->setSprayIntervalMs(80);
}

// ============================================================================
//  BUTTON SELECTION & PROPERTIES
// ============================================================================
void OverlayPanel::onButtonEditRequested(OverlayButton *btn)
{
    selectButton(btn);
    m_tabs->setCurrentIndex(3); // go to Props tab
}

void OverlayPanel::onButtonSelected(OverlayButton *btn) { selectButton(btn); }

void OverlayPanel::selectButton(OverlayButton *btn)
{
    if (m_selected) m_selected->setSelected(false);
    m_selected = btn;
    if (m_selected) m_selected->setSelected(true);
    populatePropsFromButton(m_selected);
}

void OverlayPanel::populatePropsFromButton(OverlayButton *btn)
{
    if (!m_propsWidget) return;

    if (!btn) {
        m_propsWidget->setEnabled(false);
        if (m_selTypeLabel) m_selTypeLabel->setText("No button selected");
        return;
    }

    m_propsWidget->setEnabled(true);
    if (m_selTypeLabel) {
        m_selTypeLabel->setText(QString("[%1]  %2")
            .arg(OverlayButton::typeIcon(btn->buttonType()))
            .arg(OverlayButton::typeName(btn->buttonType())));
        m_selTypeLabel->setStyleSheet("color: #38bdf8; font-weight: bold; font-size: 10px;");
    }

    if (m_labelEdit)  m_labelEdit->setText(btn->label());
    if (m_keyEdit)    m_keyEdit->setText(btn->key());
    if (m_sizeSlider) m_sizeSlider->setValue(static_cast<int>(btn->radiusRatio() * 1000));
    if (m_coordsLabel) m_coordsLabel->setText(
        QString("X: %1  Y: %2")
        .arg(btn->posRatio().x(), 0, 'f', 3)
        .arg(btn->posRatio().y(), 0, 'f', 3)
    );

    OverlayButtonType t = btn->buttonType();

    // Joystick
    bool isJoy = (t == OverlayButtonType::Joystick || t == OverlayButtonType::Vehicle);
    if (m_joyGroup) m_joyGroup->setVisible(isJoy);
    if (isJoy && m_joyUpEdit) {
        m_joyUpEdit->setText(btn->upKey());
        m_joyDownEdit->setText(btn->downKey());
        m_joyLeftEdit->setText(btn->leftKey());
        m_joyRightEdit->setText(btn->rightKey());
    }

    // Aim
    bool isAim = (t == OverlayButtonType::Aim || t == OverlayButtonType::FreeLook);
    if (m_aimGroup) m_aimGroup->setVisible(isAim);
    if (isAim && m_speedXSlider) {
        m_speedXSlider->setValue(static_cast<int>(btn->speedRatioX() * 10));
        m_speedYSlider->setValue(static_cast<int>(btn->speedRatioY() * 10));
        m_speedValLabel->setText(
            QString("X: %1  Y: %2").arg(btn->speedRatioX(), 0, 'f', 1).arg(btn->speedRatioY(), 0, 'f', 1)
        );
    }

    // Click
    bool isClick = (t == OverlayButtonType::Click || t == OverlayButtonType::RightClick ||
                    t == OverlayButtonType::MiddleClick || t == OverlayButtonType::DoubleClick);
    if (m_clickGroup) m_clickGroup->setVisible(isClick);
    if (isClick && m_switchMapCheck) m_switchMapCheck->setChecked(btn->switchMap());

    // Spray
    if (m_sprayGroup) m_sprayGroup->setVisible(t == OverlayButtonType::Spray);
    if (t == OverlayButtonType::Spray && m_sprayInterval)
        m_sprayInterval->setValue(btn->sprayIntervalMs());

    // Swipe
    bool isSwipe = (t == OverlayButtonType::Swipe || t == OverlayButtonType::Grenade);
    if (m_swipeGroup) m_swipeGroup->setVisible(isSwipe);
    if (isSwipe && m_swipeEndXEdit) {
        m_swipeEndXEdit->setText(QString::number(btn->swipeEndRatio().x(), 'f', 2));
        m_swipeEndYEdit->setText(QString::number(btn->swipeEndRatio().y(), 'f', 2));
    }

    // Macro
    if (m_macroGroup) m_macroGroup->setVisible(t == OverlayButtonType::Macro);
    if (t == OverlayButtonType::Macro && m_macroList) {
        m_macroList->clear();
        for (const auto &step : btn->macroSteps()) {
            m_macroList->addItem(
                QString("%1 %2  %3ms")
                .arg(step.press ? "PRESS" : "REL")
                .arg(step.key)
                .arg(step.delayMs)
            );
        }
    }
}

void OverlayPanel::onApplyProps()
{
    if (!m_selected) return;

    if (m_labelEdit)  m_selected->setLabel(m_labelEdit->text());
    if (m_keyEdit)    m_selected->setKey(m_keyEdit->text());
    if (m_sizeSlider) m_selected->setRadiusRatio(m_sizeSlider->value() / 1000.0f);

    OverlayButtonType t = m_selected->buttonType();

    if ((t == OverlayButtonType::Joystick || t == OverlayButtonType::Vehicle) && m_joyUpEdit) {
        m_selected->setJoystickKeys(
            m_joyUpEdit->text(), m_joyDownEdit->text(),
            m_joyLeftEdit->text(), m_joyRightEdit->text()
        );
    }
    if ((t == OverlayButtonType::Aim || t == OverlayButtonType::FreeLook) && m_speedXSlider) {
        m_selected->setSpeedRatios(m_speedXSlider->value() / 10.0f, m_speedYSlider->value() / 10.0f);
    }
    if (m_switchMapCheck && (t == OverlayButtonType::Click || t == OverlayButtonType::RightClick)) {
        m_selected->setSwitchMap(m_switchMapCheck->isChecked());
    }
    if (t == OverlayButtonType::Spray && m_sprayInterval) {
        m_selected->setSprayIntervalMs(m_sprayInterval->value());
    }
    if ((t == OverlayButtonType::Swipe || t == OverlayButtonType::Grenade) && m_swipeEndXEdit) {
        m_selected->setSwipeEndRatio(QPointF(
            m_swipeEndXEdit->text().toDouble(),
            m_swipeEndYEdit->text().toDouble()
        ));
    }
    m_selected->reposition(currentVideoGeometry());
    m_selected->update();
    emit layoutChanged();
}

void OverlayPanel::onRecordKeyClicked()
{
    m_recordingKey = true;
    if (m_recordKeyBtn) m_recordKeyBtn->setText("Press key...");
    setFocus();
    update();
}

void OverlayPanel::onMacroStepAdd()
{
    if (!m_selected || m_selected->buttonType() != OverlayButtonType::Macro) return;
    if (!m_macroKeyEdit) return;
    MacroStep step;
    step.key     = m_macroKeyEdit->text().isEmpty() ? "Key_X" : m_macroKeyEdit->text();
    step.delayMs = m_macroDelay ? m_macroDelay->value() : 50;
    step.press   = true;
    m_selected->addMacroStep(step);
    if (m_macroList) {
        m_macroList->addItem(
            QString("PRESS %1  %2ms").arg(step.key).arg(step.delayMs)
        );
    }
}

void OverlayPanel::onMacroStepRemove()
{
    if (!m_selected || !m_macroList) return;
    int row = m_macroList->currentRow();
    if (row < 0) return;
    auto steps = m_selected->macroSteps();
    if (row < steps.size()) {
        steps.removeAt(row);
        m_selected->setMacroSteps(steps);
        delete m_macroList->takeItem(row);
    }
}

void OverlayPanel::onButtonPosChanged(OverlayButton *btn)
{
    if (btn == m_selected && m_coordsLabel) {
        m_coordsLabel->setText(
            QString("X: %1  Y: %2")
            .arg(btn->posRatio().x(), 0, 'f', 3)
            .arg(btn->posRatio().y(), 0, 'f', 3)
        );
    }
}

void OverlayPanel::onButtonDeleteRequested(OverlayButton *btn)
{
    if (m_selected == btn) selectButton(nullptr);
    m_buttons.removeAll(btn);
    btn->deleteLater();
    emit layoutChanged();
}

void OverlayPanel::onButtonDuplicateRequested(OverlayButton *btn)
{
    QPointF newPos = QPointF(
        qMin(btn->posRatio().x() + 0.05, 0.95),
        qMin(btn->posRatio().y() + 0.05, 0.95)
    );
    auto *dup = createButton(btn->buttonType(), btn->label(), btn->key(), newPos);
    dup->setJoystickKeys(btn->upKey(), btn->downKey(), btn->leftKey(), btn->rightKey());
    dup->setSpeedRatios(btn->speedRatioX(), btn->speedRatioY());
    dup->setSwitchMap(btn->switchMap());
    dup->setRadiusRatio(btn->radiusRatio());
    dup->setHudOpacity(btn->hudOpacity());
    dup->setMacroSteps(btn->macroSteps());
    dup->setSprayIntervalMs(btn->sprayIntervalMs());
}

void OverlayPanel::onDeleteSelected()
{
    if (!m_selected) return;
    onButtonDeleteRequested(m_selected);
}

void OverlayPanel::onDuplicateSelected()
{
    if (!m_selected) return;
    onButtonDuplicateRequested(m_selected);
}

void OverlayPanel::onOpacitySliderChanged(int val)
{
    m_hudOpacity = val / 100.0f;
    for (auto *b : m_buttons) {
        b->setHudOpacity(m_hudOpacity);
        b->update();
    }
}

// ============================================================================
//  SAVE / LOAD / APPLY
// ============================================================================
bool OverlayPanel::saveLayout()
{
    QString dir  = userKeymapDirectory();
    QDir().mkpath(dir);
    QString path = dir + "/" + m_currentProfileName + ".json";

    QJsonObject root;
    root["switchKey"]      = m_switchKey;
    root["formatVersion"]  = 2;

    // Visual data
    QJsonArray vArr;
    for (auto *b : m_buttons) vArr.append(b->toJson());
    root["_visualButtons"] = vArr;

    // Native QtScrcpy JSON format
    QJsonArray nodes;
    QJsonObject mouseMoveMap;
    for (auto *b : m_buttons) {
        if (b->buttonType() == OverlayButtonType::Aim || b->buttonType() == OverlayButtonType::FreeLook) {
            QJsonObject sp; sp["x"] = b->posRatio().x(); sp["y"] = b->posRatio().y();
            mouseMoveMap["startPos"]   = sp;
            mouseMoveMap["speedRatioX"] = static_cast<double>(b->speedRatioX());
            mouseMoveMap["speedRatioY"] = static_cast<double>(b->speedRatioY());
        } else {
            nodes.append(b->toKeyMapNode());
        }
    }
    root["keyMapNodes"] = nodes;
    if (!mouseMoveMap.isEmpty()) root["mouseMoveMap"] = mouseMoveMap;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    m_currentJsonPath = path;
    return true;
}

void OverlayPanel::applyToDevice()
{
    if (m_currentJsonPath.isEmpty()) return;
    if (!m_refWidget) return;

    VideoForm *vf = qobject_cast<VideoForm *>(m_refWidget);
    if (!vf) return;

    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) return;

    QFile f(m_currentJsonPath);
    if (!f.open(QIODevice::ReadOnly)) return;
    QString script = QString::fromUtf8(f.readAll());
    f.close();

    device->updateScript(script);

    if (!device->isCurrentCustomKeymap()) {
        QRect vArea = currentVideoGeometry();
        QSize widgetSize = vArea.size();
        QSize frameSize  = vf->frameSize();

        int switchQtKey = stringToQtKey(m_switchKey.isEmpty() ? "Key_QuoteLeft" : m_switchKey);
        if (switchQtKey == Qt::Key_unknown) switchQtKey = Qt::Key_QuoteLeft;
        QKeyEvent pressEv(QEvent::KeyPress, switchQtKey, Qt::NoModifier);
        QKeyEvent relEv(QEvent::KeyRelease, switchQtKey, Qt::NoModifier);
        emit device->keyEvent(&pressEv, frameSize, widgetSize);
        emit device->keyEvent(&relEv, frameSize, widgetSize);
    }
}

void OverlayPanel::loadLayout(const QString &jsonPath)
{
    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    m_currentJsonPath = jsonPath;

    for (auto *b : m_buttons) b->deleteLater();
    m_buttons.clear();
    selectButton(nullptr);

    if (root.contains("switchKey")) m_switchKey = root["switchKey"].toString("Key_QuoteLeft");

    if (root.contains("_visualButtons")) {
        for (const auto &val : root["_visualButtons"].toArray()) {
            auto *b = OverlayButton::fromJson(val.toObject(), this);
            b->setEditMode(m_editMode);
            b->reposition(currentVideoGeometry());
            connect(b, &OverlayButton::editRequested,      this, &OverlayPanel::onButtonEditRequested);
            connect(b, &OverlayButton::posRatioChanged,    this, &OverlayPanel::onButtonPosChanged);
            connect(b, &OverlayButton::deleteRequested,    this, &OverlayPanel::onButtonDeleteRequested);
            connect(b, &OverlayButton::duplicateRequested, this, &OverlayPanel::onButtonDuplicateRequested);
            m_buttons.append(b);
            b->show();
        }
    } else if (root.contains("keyMapNodes")) {
        if (root.contains("mouseMoveMap")) {
            QJsonObject mm = root["mouseMoveMap"].toObject();
            QJsonObject sp = mm["startPos"].toObject();
            auto *aimBtn = createButton(OverlayButtonType::Aim, "Aim", m_switchKey,
                                        QPointF(sp["x"].toDouble(0.65), sp["y"].toDouble(0.45)));
            aimBtn->setSpeedRatios(static_cast<float>(mm["speedRatioX"].toDouble(2.5)),
                                   static_cast<float>(mm["speedRatioY"].toDouble(2.5)));
        }
        for (const auto &val : root["keyMapNodes"].toArray()) {
            QJsonObject node = val.toObject();
            QString type = node["type"].toString();
            QString comment = node["comment"].toString();
            if (type == "KMT_CLICK") {
                QJsonObject pos = node["pos"].toObject();
                auto *btn = createButton(OverlayButtonType::Click, comment, node["key"].toString(),
                                          QPointF(pos["x"].toDouble(), pos["y"].toDouble()));
                btn->setSwitchMap(node["switchMap"].toBool(false));
            } else if (type == "KMT_CLICK_TWICE") {
                QJsonObject pos = node["pos"].toObject();
                createButton(OverlayButtonType::DoubleClick, comment, node["key"].toString(),
                             QPointF(pos["x"].toDouble(), pos["y"].toDouble()));
            } else if (type == "KMT_STEER_WHEEL") {
                QJsonObject cp = node["centerPos"].toObject();
                auto *joy = createButton(OverlayButtonType::Joystick, comment, "WASD",
                                          QPointF(cp["x"].toDouble(), cp["y"].toDouble()));
                joy->setJoystickKeys(node["upKey"].toString("Key_W"), node["downKey"].toString("Key_S"),
                                     node["leftKey"].toString("Key_A"), node["rightKey"].toString("Key_D"));
            } else if (type == "KMT_DRAG") {
                QJsonObject sp = node["startPos"].toObject();
                createButton(OverlayButtonType::Swipe, comment, node["key"].toString(),
                             QPointF(sp["x"].toDouble(), sp["y"].toDouble()));
            }
        }
    }
    emit layoutChanged();
    update();
}

// ============================================================================
//  PROFILE MANAGEMENT
// ============================================================================
void OverlayPanel::refreshProfileList()
{
    if (!m_presetCombo) return;
    m_presetCombo->blockSignals(true);
    m_presetCombo->clear();

    QString dir = userKeymapDirectory();
    QDir d(dir);
    QStringList files = d.entryList({"*.json"}, QDir::Files);
    for (const auto &f : files) m_presetCombo->addItem(QFileInfo(f).baseName());

    if (m_presetCombo->count() == 0) m_presetCombo->addItem("custom_keymap");
    m_presetCombo->blockSignals(false);
}

void OverlayPanel::onProfilePresetSelected(int index)
{
    if (index < 0 || !m_presetCombo) return;
    m_currentProfileName = m_presetCombo->itemText(index);
    if (m_profileEdit) m_profileEdit->setText(m_currentProfileName);
    QString path = userKeymapDirectory() + "/" + m_currentProfileName + ".json";
    if (QFile::exists(path)) loadLayout(path);
}

// ============================================================================
//  SAVE / IMPORT / EXPORT / CLEAR SLOTS
// ============================================================================
void OverlayPanel::onSaveAndApply()
{
    if (m_profileEdit && !m_profileEdit->text().isEmpty())
        m_currentProfileName = m_profileEdit->text();

    saveLayout();
    applyToDevice();
    setEditMode(false);
    setOverlayVisible(true);
    refreshProfileList();

    m_toastMessage = "Keymap Applied & Active!";
    update();
    QTimer::singleShot(3000, this, [this]() { m_toastMessage.clear(); update(); });
}

void OverlayPanel::onToggleHUD()  { setOverlayVisible(!m_overlayOn); }
void OverlayPanel::onCloseEdit()  { setEditMode(false); }

void OverlayPanel::onExportKeymap()
{
    saveLayout();
    QString src = userKeymapDirectory() + "/" + m_currentProfileName + ".json";
    QString dest = QFileDialog::getSaveFileName(this, "Export Keymap", m_currentProfileName + ".json", "JSON (*.json)");
    if (!dest.isEmpty()) {
        if (QFile::exists(dest)) QFile::remove(dest);
        bool ok = QFile::copy(src, dest);
        m_toastMessage = ok ? "Exported successfully!" : "Export failed!";
        update();
        QTimer::singleShot(3000, this, [this]() { m_toastMessage.clear(); update(); });
    }
}

void OverlayPanel::onImportKeymap()
{
    QString src = QFileDialog::getOpenFileName(this, "Import Keymap", "", "JSON (*.json)");
    if (!src.isEmpty()) {
        QString dest = userKeymapDirectory() + "/" + QFileInfo(src).baseName() + ".json";
        if (QFile::exists(dest)) QFile::remove(dest);
        if (QFile::copy(src, dest)) {
            m_currentProfileName = QFileInfo(src).baseName();
            refreshProfileList();
            loadLayout(dest);
            m_toastMessage = "Keymap imported!";
        } else {
            m_toastMessage = "Import failed!";
        }
        update();
        QTimer::singleShot(3000, this, [this]() { m_toastMessage.clear(); update(); });
    }
}

void OverlayPanel::onClearAll()
{
    if (QMessageBox::question(this, "Clear All", "Remove all buttons?") != QMessageBox::Yes) return;
    for (auto *b : m_buttons) b->deleteLater();
    m_buttons.clear();
    selectButton(nullptr);
    emit layoutChanged();
}

// ============================================================================
//  HELPERS
// ============================================================================
QString OverlayPanel::userKeymapDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/keymap";
}

QString OverlayPanel::defaultKeymapDirectory() const
{
    return QCoreApplication::applicationDirPath() + "/keymap";
}

QString OverlayPanel::qtKeyToString(int key)
{
    if (key == Qt::Key_Space)      return "Key_Space";
    if (key == Qt::Key_Tab)        return "Key_Tab";
    if (key == Qt::Key_Escape)     return "Key_Escape";
    if (key == Qt::Key_Return)     return "Key_Return";
    if (key == Qt::Key_Backspace)  return "Key_Backspace";
    if (key == Qt::Key_Shift)      return "Key_Shift";
    if (key == Qt::Key_Control)    return "Key_Control";
    if (key == Qt::Key_Alt)        return "Key_Alt";
    if (key == Qt::Key_F1)         return "Key_F1";
    if (key == Qt::Key_F2)         return "Key_F2";
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        return QString("Key_") + QChar(key);
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return QString("Key_") + QChar(key);
    if (key == Qt::Key_QuoteLeft)  return "Key_QuoteLeft";
    if (key == Qt::Key_Minus)      return "Key_Minus";
    if (key == Qt::Key_Equal)      return "Key_Equal";
    if (key == Qt::Key_BracketLeft)return "Key_BracketLeft";
    if (key == Qt::Key_BracketRight) return "Key_BracketRight";
    if (key == Qt::Key_Semicolon)  return "Key_Semicolon";
    if (key == Qt::Key_Apostrophe) return "Key_Apostrophe";
    if (key == Qt::Key_Comma)      return "Key_Comma";
    if (key == Qt::Key_Period)     return "Key_Period";
    if (key == Qt::Key_Slash)      return "Key_Slash";
    if (key == Qt::Key_Backslash)  return "Key_Backslash";
    return QString("Key_%1").arg(key);
}

int OverlayPanel::stringToQtKey(const QString &s)
{
    if (s == "Key_Space")      return Qt::Key_Space;
    if (s == "Key_Tab")        return Qt::Key_Tab;
    if (s == "Key_Return")     return Qt::Key_Return;
    if (s == "Key_Escape")     return Qt::Key_Escape;
    if (s == "Key_Backspace")  return Qt::Key_Backspace;
    if (s == "Key_Shift")      return Qt::Key_Shift;
    if (s == "Key_Control")    return Qt::Key_Control;
    if (s == "Key_Alt")        return Qt::Key_Alt;
    if (s == "Key_QuoteLeft")  return Qt::Key_QuoteLeft;
    if (s.startsWith("Key_") && s.length() == 5) {
        QChar c = s[4];
        if (c.isLetter()) return c.toUpper().unicode();
        if (c.isDigit())  return c.unicode();
    }
    return Qt::Key_unknown;
}

QString OverlayPanel::keyToDisplayLabel(const QString &k)
{
    if (k.startsWith("Key_")) return k.mid(4);
    return k;
}





