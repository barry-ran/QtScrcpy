#include "overlaypanel.h"
#include <QFileDialog>
#include <QMessageBox>
#include "videoform.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>

// ---------------------------------------------------------------------------
// Construction & Destruction
// ---------------------------------------------------------------------------
OverlayPanel::OverlayPanel(const QString &serial, QWidget *parent)
    : QWidget(parent)
    , m_serial(serial)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);

    buildSidePanel();
    setEditMode(false);
    refreshProfileList();

    // Default sample buttons if clean start
    if (m_buttons.isEmpty()) {
        createButton(OverlayButtonType::Joystick, "Move", "Key_W", QPointF(0.18, 0.72));
        createButton(OverlayButtonType::Aim, "Aim", "Key_QuoteLeft", QPointF(0.65, 0.45));
        createButton(OverlayButtonType::Click, "Fire", "Key_J", QPointF(0.82, 0.68));
        createButton(OverlayButtonType::Click, "Jump", "Key_Space", QPointF(0.88, 0.82));
    }
}

OverlayPanel::~OverlayPanel()
{
}

// ---------------------------------------------------------------------------
// Paths
// ---------------------------------------------------------------------------
QString OverlayPanel::userKeymapDirectory() const
{
    QString dir = QString::fromLocal8Bit(qgetenv("QTSCRCPY_KEYMAP_PATH"));
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/keymap";
    }
    QDir().mkpath(dir);
    return dir;
}

QString OverlayPanel::defaultKeymapDirectory() const
{
    return QCoreApplication::applicationDirPath() + "/keymap";
}

// ---------------------------------------------------------------------------
// Video Geometry
// ---------------------------------------------------------------------------
QRect OverlayPanel::currentVideoGeometry() const
{
    VideoForm *vf = qobject_cast<VideoForm *>(parentWidget());
    if (vf && vf->videoWidget()) {
        QWidget *vw = vf->videoWidget();
        QPoint topLeft = mapFromGlobal(vw->mapToGlobal(QPoint(0, 0)));
        return QRect(topLeft, vw->size());
    }
    return rect();
}

void OverlayPanel::onParentResized()
{
    if (parentWidget()) {
        setGeometry(parentWidget()->rect());
    }
    updateSidePanelGeometry();
    QRect videoArea = currentVideoGeometry();
    for (auto *b : m_buttons) {
        if (b) b->reposition(videoArea);
    }
    update();
}

void OverlayPanel::resizeEvent(QResizeEvent *)
{
    onParentResized();
}

// ---------------------------------------------------------------------------
// Edit vs Play Modes
// ---------------------------------------------------------------------------
void OverlayPanel::setEditMode(bool edit)
{
    m_editMode = edit;
    m_recordingKey = false;

    // Critical: in Play mode, pass ALL mouse clicks directly to VideoForm
    setAttribute(Qt::WA_TransparentForMouseEvents, !edit);

    if (edit) {
        setFocusPolicy(Qt::StrongFocus);
        setFocus();
        m_sidePanel->show();
        m_sidePanel->raise();
        for (auto *b : m_buttons) {
            if (b) {
                b->setEditMode(true);
                b->show();
            }
        }
    } else {
        setFocusPolicy(Qt::NoFocus);
        clearFocus();
        m_sidePanel->hide();
        selectButton(nullptr);
        for (auto *b : m_buttons) {
            if (b) {
                b->setEditMode(false);
                b->setVisible(m_overlayOn);
                // Also ensure buttons don't block mouse in play mode
                b->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            }
        }
        if (parentWidget()) {
            parentWidget()->setFocus();
        }
    }
    update();
}

void OverlayPanel::setOverlayVisible(bool v)
{
    m_overlayOn = v;
    if (!m_editMode) {
        for (auto *b : m_buttons) {
            if (b) b->setVisible(m_overlayOn);
        }
    }
    if (m_hudToggleBtn) {
        m_hudToggleBtn->setText(m_overlayOn ? tr(" HUD: ON") : tr(" HUD: OFF"));
    }
    update();
}

// ---------------------------------------------------------------------------
// Button Factory
// ---------------------------------------------------------------------------
OverlayButton *OverlayPanel::createButton(OverlayButtonType type,
                                         const QString &label,
                                         const QString &key,
                                         QPointF posRatio)
{
    auto *btn = new OverlayButton(type, label, key, posRatio, this);
    btn->setEditMode(m_editMode);
    btn->reposition(currentVideoGeometry());

    connect(btn, &OverlayButton::editRequested,      this, &OverlayPanel::onButtonEditRequested);
    connect(btn, &OverlayButton::posRatioChanged,    this, &OverlayPanel::onButtonPosChanged);
    connect(btn, &OverlayButton::deleteRequested,    this, &OverlayPanel::onButtonDeleteRequested);
    connect(btn, &OverlayButton::duplicateRequested, this, &OverlayPanel::onButtonDuplicateRequested);

    m_buttons.append(btn);
    btn->show();
    return btn;
}

// ---------------------------------------------------------------------------
// Side Panel UI Construction
// ---------------------------------------------------------------------------
void OverlayPanel::buildSidePanel()
{
    m_sidePanel = new QFrame(this);
    m_sidePanel->setObjectName("sidePanel");

    // Modern Deep Slate / Cyber Gaming Theme with sleek scrollbar
    m_sidePanel->setStyleSheet(
        "#sidePanel {"
        "  background-color: rgba(15, 23, 42, 0.96);"
        "  border-left: 2px solid #334155;"
        "  border-top-left-radius: 12px;"
        "  border-bottom-left-radius: 12px;"
        "}"
        "QLabel {"
        "  color: #f1f5f9;"
        "  font-family: 'Segoe UI', Arial, sans-serif;"
        "}"
        "QLineEdit, QComboBox {"
        "  background-color: #1e293b;"
        "  border: 1px solid #475569;"
        "  border-radius: 6px;"
        "  color: #f8fafc;"
        "  padding: 4px 6px;"
        "  font-size: 11px;"
        "}"
        "QLineEdit:focus, QComboBox:focus {"
        "  border: 1px solid #38bdf8;"
        "}"
        "QPushButton {"
        "  background-color: #334155;"
        "  border: 1px solid #475569;"
        "  border-radius: 6px;"
        "  color: #f8fafc;"
        "  padding: 5px 8px;"
        "  font-weight: 600;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #475569;"
        "  border-color: #64748b;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1e293b;"
        "}"
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: #334155;"
        "  border-radius: 2px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: #38bdf8;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #f8fafc;"
        "  width: 12px;"
        "  margin-top: -4px;"
        "  margin-bottom: -4px;"
        "  border-radius: 6px;"
        "}"
        "QCheckBox { color: #cbd5e1; font-size: 11px; }"
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical {"
        "  background: rgba(30, 41, 59, 0.4);"
        "  width: 5px;"
        "  margin: 0px;"
        "  border-radius: 2px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #475569;"
        "  min-height: 20px;"
        "  border-radius: 2px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #38bdf8;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
    );

    auto *mainLayout = new QVBoxLayout(m_sidePanel);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // --- Header ---
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(2, 2, 2, 2);
    auto *titleLabel = new QLabel(tr(" KEYMAP STUDIO"), m_sidePanel);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #38bdf8; letter-spacing: 0.5px;");
    headerLayout->addWidget(titleLabel);

    auto *badgeLabel = new QLabel(tr("PRO"), m_sidePanel);
    badgeLabel->setStyleSheet("background-color: #0284c7; color: white; font-size: 9px; font-weight: bold; padding: 1px 5px; border-radius: 4px;");
    headerLayout->addWidget(badgeLabel);
    headerLayout->addStretch(1);
    mainLayout->addLayout(headerLayout);

    // Profile presets
    auto *presetRow = new QHBoxLayout();
    presetRow->setSpacing(4);
    m_presetCombo = new QComboBox(m_sidePanel);
    m_presetCombo->setToolTip(tr("Select an existing keymap preset"));
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OverlayPanel::onProfilePresetSelected);
    presetRow->addWidget(m_presetCombo, 1);

    m_profileEdit = new QLineEdit(m_currentProfileName, m_sidePanel);
    m_profileEdit->setPlaceholderText(tr("Profile Name"));
    presetRow->addWidget(m_profileEdit, 1);
    mainLayout->addLayout(presetRow);

    // --- Scroll Area for Dynamic Content Auto-Resize ---
    auto *scrollArea = new QScrollArea(m_sidePanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    auto *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(2, 4, 4, 4);
    contentLayout->setSpacing(8);

    // --- Quick Add Arsenal (TC Games & WASD+ Feature Set) ---
    auto *addLabel = new QLabel(tr("ADD ELEMENTS (TC/WASD+ SYSTEM)"), scrollContent);
    addLabel->setStyleSheet("font-size: 10px; font-weight: bold; color: #94a3b8; letter-spacing: 0.5px; margin-top: 2px;");
    contentLayout->addWidget(addLabel);

    auto *addGrid = new QGridLayout();
    addGrid->setSpacing(4);

    // Dedicated Fire button (Left Click)
    auto *addFireBtn = new QPushButton(tr(" Fire (L-Click)"), scrollContent);
    addFireBtn->setStyleSheet("background-color: #991b1b; border-color: #ef4444; color: white;");
    addFireBtn->setToolTip(tr("Weapon fire button mapped to Mouse Left Click"));
    connect(addFireBtn, &QPushButton::clicked, this, &OverlayPanel::onAddFire);

    // Dedicated Scope button (Right Click)
    auto *addScopeBtn = new QPushButton(tr(" Scope (R-Click)"), scrollContent);
    addScopeBtn->setStyleSheet("background-color: #0f766e; border-color: #14b8a6; color: white;");
    addScopeBtn->setToolTip(tr("Aim down sights (ADS) mapped to Mouse Right Click"));
    connect(addScopeBtn, &QPushButton::clicked, this, &OverlayPanel::onAddScope);

    // WASD Joystick
    auto *addJoyBtn = new QPushButton(tr(" WASD Move"), scrollContent);
    addJoyBtn->setStyleSheet("background-color: #0369a1; border-color: #38bdf8; color: white;");
    addJoyBtn->setToolTip(tr("360 degree virtual analog joystick"));
    connect(addJoyBtn, &QPushButton::clicked, this, &OverlayPanel::onAddJoystick);

    // Aim / Mouse Look
    auto *addAimBtn = new QPushButton(tr(" Aim / Look"), scrollContent);
    addAimBtn->setStyleSheet("background-color: #b91c1c; border-color: #f87171; color: white;");
    addAimBtn->setToolTip(tr("FPS camera look and mouse aiming with customizable sensitivity"));
    connect(addAimBtn, &QPushButton::clicked, this, &OverlayPanel::onAddAim);

    // Free Look (Alt Eye)
    auto *addEyeBtn = new QPushButton(tr(" Free Look (Alt)"), scrollContent);
    addEyeBtn->setStyleSheet("background-color: #854d0e; border-color: #eab308; color: white;");
    addEyeBtn->setToolTip(tr("360-degree observation look (Small Eye)"));
    connect(addEyeBtn, &QPushButton::clicked, this, &OverlayPanel::onAddFreeLook);

    // Rapid Fire (Turbo Multi Click)
    auto *addRapidBtn = new QPushButton(tr(" Rapid Fire"), scrollContent);
    addRapidBtn->setStyleSheet("background-color: #6b21a8; border-color: #a855f7; color: white;");
    addRapidBtn->setToolTip(tr("Double / Rapid click for semi-automatic guns"));
    connect(addRapidBtn, &QPushButton::clicked, this, &OverlayPanel::onAddDoubleClick);

    // Map (M with switchMap)
    auto *addMapBtn = new QPushButton(tr(" Map (M)"), scrollContent);
    addMapBtn->setStyleSheet("background-color: #1e3a8a; border-color: #3b82f6; color: white;");
    addMapBtn->setToolTip(tr("Map button with automatic cursor release (switchMap)"));
    connect(addMapBtn, &QPushButton::clicked, this, &OverlayPanel::onAddMap);

    // Bag (Tab with switchMap)
    auto *addBagBtn = new QPushButton(tr(" Bag (Tab)"), scrollContent);
    addBagBtn->setStyleSheet("background-color: #374151; border-color: #9ca3af; color: white;");
    addBagBtn->setToolTip(tr("Backpack / Inventory button with cursor release"));
    connect(addBagBtn, &QPushButton::clicked, this, &OverlayPanel::onAddBag);

    // Normal Click
    auto *addClickBtn = new QPushButton(tr(" Key Click"), scrollContent);
    addClickBtn->setToolTip(tr("Standard button click"));
    connect(addClickBtn, &QPushButton::clicked, this, &OverlayPanel::onAddClick);

    // Swipe / Slide
    auto *addSwipeBtn = new QPushButton(tr(" Swipe"), scrollContent);
    addSwipeBtn->setToolTip(tr("Drag gesture for sliding / dodging"));
    connect(addSwipeBtn, &QPushButton::clicked, this, &OverlayPanel::onAddSwipe);

    addGrid->addWidget(addFireBtn,  0, 0);
    addGrid->addWidget(addScopeBtn, 0, 1);
    addGrid->addWidget(addJoyBtn,   1, 0);
    addGrid->addWidget(addAimBtn,   1, 1);
    addGrid->addWidget(addEyeBtn,   2, 0);
    addGrid->addWidget(addRapidBtn, 2, 1);
    addGrid->addWidget(addMapBtn,   3, 0);
    addGrid->addWidget(addBagBtn,   3, 1);
    addGrid->addWidget(addClickBtn, 4, 0);
    addGrid->addWidget(addSwipeBtn, 4, 1);
    contentLayout->addLayout(addGrid);

    // --- Properties Inspector ---
    auto *sep = new QFrame(scrollContent);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #334155; margin: 4px 0;");
    contentLayout->addWidget(sep);

    auto *propsLabel = new QLabel(tr("PROPERTIES INSPECTOR"), scrollContent);
    propsLabel->setStyleSheet("font-size: 10px; font-weight: bold; color: #94a3b8; letter-spacing: 0.5px;");
    contentLayout->addWidget(propsLabel);

    m_propsWidget = new QWidget(scrollContent);
    auto *pLayout = new QVBoxLayout(m_propsWidget);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(6);

    m_selTypeLabel = new QLabel(tr("No button selected"), m_propsWidget);
    m_selTypeLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #38bdf8;");
    pLayout->addWidget(m_selTypeLabel);

    // Label edit
    auto *labelRow = new QHBoxLayout();
    labelRow->addWidget(new QLabel(tr("Label:"), m_propsWidget));
    m_labelEdit = new QLineEdit(m_propsWidget);
    connect(m_labelEdit, &QLineEdit::textChanged, this, &OverlayPanel::onApplyProps);
    labelRow->addWidget(m_labelEdit);
    pLayout->addLayout(labelRow);

    // Interactive Key Recorder
    auto *keyRow = new QHBoxLayout();
    keyRow->addWidget(new QLabel(tr("Key:"), m_propsWidget));
    m_recordKeyBtn = new QPushButton(tr(" Bind Key"), m_propsWidget);
    m_recordKeyBtn->setStyleSheet("background-color: #0284c7; color: white; font-weight: bold;");
    connect(m_recordKeyBtn, &QPushButton::clicked, this, &OverlayPanel::onRecordKeyClicked);
    keyRow->addWidget(m_recordKeyBtn);

    m_keyEdit = new QLineEdit(m_propsWidget);
    m_keyEdit->setFixedWidth(75);
    m_keyEdit->setPlaceholderText(tr("Key_*"));
    connect(m_keyEdit, &QLineEdit::textChanged, this, &OverlayPanel::onApplyProps);
    keyRow->addWidget(m_keyEdit);
    pLayout->addLayout(keyRow);

    // Quick Mouse Buttons Selector (1-click assign Left / Right / Mid click)
    auto *mouseRow = new QHBoxLayout();
    mouseRow->setSpacing(3);
    auto *setLeftBtn = new QPushButton(tr(" L-Click (Fire)"), m_propsWidget);
    setLeftBtn->setStyleSheet("background-color: #991b1b; color: white; font-size: 10px; padding: 3px;");
    connect(setLeftBtn, &QPushButton::clicked, this, &OverlayPanel::onSetLeftClick);

    auto *setRightBtn = new QPushButton(tr(" R-Click (Scope)"), m_propsWidget);
    setRightBtn->setStyleSheet("background-color: #0f766e; color: white; font-size: 10px; padding: 3px;");
    connect(setRightBtn, &QPushButton::clicked, this, &OverlayPanel::onSetRightClick);

    auto *setMidBtn = new QPushButton(tr(" Mid"), m_propsWidget);
    setMidBtn->setStyleSheet("background-color: #334155; color: white; font-size: 10px; padding: 3px;");
    connect(setMidBtn, &QPushButton::clicked, this, &OverlayPanel::onSetMidClick);

    mouseRow->addWidget(setLeftBtn);
    mouseRow->addWidget(setRightBtn);
    mouseRow->addWidget(setMidBtn);
    pLayout->addLayout(mouseRow);

    // Size slider
    auto *sizeRow = new QHBoxLayout();
    sizeRow->addWidget(new QLabel(tr("Size:"), m_propsWidget));
    m_sizeSlider = new QSlider(Qt::Horizontal, m_propsWidget);
    m_sizeSlider->setRange(20, 200);
    m_sizeSlider->setValue(55);
    connect(m_sizeSlider, &QSlider::valueChanged, this, &OverlayPanel::onApplyProps);
    sizeRow->addWidget(m_sizeSlider);
    pLayout->addLayout(sizeRow);

    // HUD Opacity Slider
    auto *opRow = new QHBoxLayout();
    opRow->addWidget(new QLabel(tr("HUD Opacity:"), m_propsWidget));
    m_opacitySlider = new QSlider(Qt::Horizontal, m_propsWidget);
    m_opacitySlider->setRange(15, 100);
    m_opacitySlider->setValue(85);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &OverlayPanel::onOpacitySliderChanged);
    opRow->addWidget(m_opacitySlider);
    pLayout->addLayout(opRow);

    // Coords label
    m_coordsLabel = new QLabel("X: 0.50 | Y: 0.50", m_propsWidget);
    m_coordsLabel->setStyleSheet("color: #64748b; font-size: 10px;");
    pLayout->addWidget(m_coordsLabel);

    // Joystick keys group
    m_joyGroup = new QWidget(m_propsWidget);
    auto *jg = new QGridLayout(m_joyGroup);
    jg->setContentsMargins(0, 4, 0, 4);
    jg->setSpacing(4);
    m_joyUpEdit    = new QLineEdit("Key_W", m_joyGroup);
    m_joyDownEdit  = new QLineEdit("Key_S", m_joyGroup);
    m_joyLeftEdit  = new QLineEdit("Key_A", m_joyGroup);
    m_joyRightEdit = new QLineEdit("Key_D", m_joyGroup);
    jg->addWidget(new QLabel("Up:"), 0, 0);    jg->addWidget(m_joyUpEdit, 0, 1);
    jg->addWidget(new QLabel("Down:"), 0, 2);  jg->addWidget(m_joyDownEdit, 0, 3);
    jg->addWidget(new QLabel("Left:"), 1, 0);  jg->addWidget(m_joyLeftEdit, 1, 1);
    jg->addWidget(new QLabel("Right:"), 1, 2); jg->addWidget(m_joyRightEdit, 1, 3);
    connect(m_joyUpEdit,    &QLineEdit::textChanged, this, &OverlayPanel::onApplyProps);
    connect(m_joyDownEdit,  &QLineEdit::textChanged, this, &OverlayPanel::onApplyProps);
    connect(m_joyLeftEdit,  &QLineEdit::textChanged, this, &OverlayPanel::onApplyProps);
    connect(m_joyRightEdit, &QLineEdit::textChanged, this, &OverlayPanel::onApplyProps);

    auto *resetWasdBtn = new QPushButton(tr(" Reset Standard WASD"), m_joyGroup);
    resetWasdBtn->setStyleSheet("background-color: #0369a1; border-color: #0284c7; padding: 4px; font-size: 11px;");
    connect(resetWasdBtn, &QPushButton::clicked, this, [this]() {
        if (!m_selected || m_selected->buttonType() != OverlayButtonType::Joystick) return;
        m_joyUpEdit->setText("Key_W");
        m_joyDownEdit->setText("Key_S");
        m_joyLeftEdit->setText("Key_A");
        m_joyRightEdit->setText("Key_D");
        onApplyProps();
    });
    jg->addWidget(resetWasdBtn, 2, 0, 1, 4);
    pLayout->addWidget(m_joyGroup);

    // Aim sensitivity group
    m_aimGroup = new QWidget(m_propsWidget);
    auto *ag = new QVBoxLayout(m_aimGroup);
    ag->setContentsMargins(0, 4, 0, 4);
    ag->setSpacing(4);
    m_speedValLabel = new QLabel(tr("Aim Sensitivity: 2.5x"), m_aimGroup);
    m_speedValLabel->setStyleSheet("font-size: 11px; color: #cbd5e1;");
    ag->addWidget(m_speedValLabel);
    m_speedXSlider = new QSlider(Qt::Horizontal, m_aimGroup);
    m_speedXSlider->setRange(5, 50);
    m_speedXSlider->setValue(25);
    connect(m_speedXSlider, &QSlider::valueChanged, this, &OverlayPanel::onApplyProps);
    ag->addWidget(m_speedXSlider);

    auto *aimPresetsLabel = new QLabel(tr("Aim Toggle Key (Lock/Unlock):"), m_aimGroup);
    aimPresetsLabel->setStyleSheet("font-size: 11px; color: #94a3b8; font-weight: 600; margin-top: 4px;");
    ag->addWidget(aimPresetsLabel);

    auto *aimPresetsLayout = new QHBoxLayout();
    aimPresetsLayout->setSpacing(4);
    auto *rcBtn = new QPushButton(tr(" Right Click"), m_aimGroup);
    rcBtn->setStyleSheet("background-color: #1e3a8a; border-color: #3b82f6; font-size: 11px; padding: 4px;");
    connect(rcBtn, &QPushButton::clicked, this, [this]() {
        if (!m_selected) return;
        m_keyEdit->setText("RightButton");
        m_switchKey = "RightButton";
        onApplyProps();
    });
    auto *tildeBtn = new QPushButton(tr("~ Tilde"), m_aimGroup);
    tildeBtn->setStyleSheet("background-color: #334155; border-color: #475569; font-size: 11px; padding: 4px;");
    connect(tildeBtn, &QPushButton::clicked, this, [this]() {
        if (!m_selected) return;
        m_keyEdit->setText("Key_QuoteLeft");
        m_switchKey = "Key_QuoteLeft";
        onApplyProps();
    });
    auto *vBtn = new QPushButton(tr("V Key"), m_aimGroup);
    vBtn->setStyleSheet("background-color: #334155; border-color: #475569; font-size: 11px; padding: 4px;");
    connect(vBtn, &QPushButton::clicked, this, [this]() {
        if (!m_selected) return;
        m_keyEdit->setText("Key_V");
        m_switchKey = "Key_V";
        onApplyProps();
    });
    aimPresetsLayout->addWidget(rcBtn);
    aimPresetsLayout->addWidget(tildeBtn);
    aimPresetsLayout->addWidget(vBtn);
    ag->addLayout(aimPresetsLayout);
    pLayout->addWidget(m_aimGroup);

    // Click switchMap group
    m_clickGroup = new QWidget(m_propsWidget);
    auto *cg = new QHBoxLayout(m_clickGroup);
    cg->setContentsMargins(0, 2, 0, 2);
    m_switchMapCheck = new QCheckBox(tr("Release mouse cursor (switchMap)"), m_clickGroup);
    connect(m_switchMapCheck, &QCheckBox::toggled, this, &OverlayPanel::onApplyProps);
    cg->addWidget(m_switchMapCheck);
    pLayout->addWidget(m_clickGroup);

    // Action buttons row (Duplicate / Delete)
    auto *actRow = new QHBoxLayout();
    auto *dupBtn = new QPushButton(tr(" Duplicate"), m_propsWidget);
    dupBtn->setStyleSheet("background-color: #334155; color: white;");
    connect(dupBtn, &QPushButton::clicked, this, &OverlayPanel::onDuplicateSelected);
    actRow->addWidget(dupBtn);

    m_deleteBtn = new QPushButton(tr(" Delete"), m_propsWidget);
    m_deleteBtn->setStyleSheet("background-color: #dc2626; color: white;");
    connect(m_deleteBtn, &QPushButton::clicked, this, &OverlayPanel::onDeleteSelected);
    actRow->addWidget(m_deleteBtn);
    pLayout->addLayout(actRow);

    contentLayout->addWidget(m_propsWidget);
    contentLayout->addStretch(1);

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // --- Bottom Action Bar (Fixed at bottom) ---
    m_saveBtn = new QPushButton(tr(" Save & Apply"), m_sidePanel);
    m_saveBtn->setFixedHeight(36);
    m_saveBtn->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #059669, stop:1 #10b981);"
        "  color: white;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 1px solid #34d399;"
        "  border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #10b981, stop:1 #34d399);"
        "}"
    );
    connect(m_saveBtn, &QPushButton::clicked, this, &OverlayPanel::onSaveAndApply);
    mainLayout->addWidget(m_saveBtn);

    // Export JSON Button
        auto *fileBtnsLayout = new QHBoxLayout();
    auto *exportBtn = new QPushButton(tr("Export JSON"), m_sidePanel);
    exportBtn->setFixedHeight(32);
    exportBtn->setStyleSheet("QPushButton { background: #3b82f6; color: white; border-radius: 4px; font-weight: bold; } QPushButton:hover { background: #2563eb; }");
    connect(exportBtn, &QPushButton::clicked, this, &OverlayPanel::onExportKeymap);
    
    m_importBtn = new QPushButton(tr("Import JSON"), m_sidePanel);
    m_importBtn->setFixedHeight(32);
    m_importBtn->setStyleSheet("QPushButton { background: #8b5cf6; color: white; border-radius: 4px; font-weight: bold; } QPushButton:hover { background: #7c3aed; }");
    connect(m_importBtn, &QPushButton::clicked, this, &OverlayPanel::onImportKeymap);
    
    fileBtnsLayout->addWidget(exportBtn);
    fileBtnsLayout->addWidget(m_importBtn);
    mainLayout->addLayout(fileBtnsLayout);
    
    m_clearAllBtn = new QPushButton(tr("Clear All"), m_sidePanel);
    m_clearAllBtn->setFixedHeight(32);
    m_clearAllBtn->setStyleSheet("QPushButton { background: #ef4444; color: white; border-radius: 4px; font-weight: bold; } QPushButton:hover { background: #dc2626; }");
    connect(m_clearAllBtn, &QPushButton::clicked, this, &OverlayPanel::onClearAll);
    mainLayout->addWidget(m_clearAllBtn);

    auto *bottomRow = new QHBoxLayout();
    m_hudToggleBtn = new QPushButton(tr(" HUD: ON"), m_sidePanel);
    connect(m_hudToggleBtn, &QPushButton::clicked, this, &OverlayPanel::onToggleHUD);
    bottomRow->addWidget(m_hudToggleBtn);

    m_closeBtn = new QPushButton(tr(" Close"), m_sidePanel);
    connect(m_closeBtn, &QPushButton::clicked, this, &OverlayPanel::onCloseEdit);
    bottomRow->addWidget(m_closeBtn);

    mainLayout->addLayout(bottomRow);

    m_propsWidget->setEnabled(false);
}

void OverlayPanel::updateSidePanelGeometry()
{
    if (m_sidePanel) {
        int panelW = qBound(280, int(width() * 0.32), 340);
        m_sidePanel->setGeometry(width() - panelW, 0, panelW, height());
    }
}

// ---------------------------------------------------------------------------
// Painting Canvas
// ---------------------------------------------------------------------------
void OverlayPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (m_editMode) {
        // Subtle dark translucent veil over screen
        p.fillRect(rect(), QColor(15, 23, 42, 110));

        // Phone Video Area Boundary Highlight
        QRect vArea = currentVideoGeometry();
        p.setPen(QPen(QColor(56, 189, 248, 180), 2, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(vArea, 6, 6);

        // Top Guide Banner
        int bannerW = qMin(width() - 320, 520);
        QRect bannerRect((width() - 290 - bannerW) / 2, 12, bannerW, 30);
        p.setPen(QPen(QColor(51, 65, 85, 220), 1));
        p.setBrush(QColor(15, 23, 42, 210));
        p.drawRoundedRect(bannerRect, 6, 6);

        QFont bFont = p.font();
        bFont.setPointSize(9);
        bFont.setBold(true);
        p.setFont(bFont);
        p.setPen(QColor(226, 232, 240));
        p.drawText(bannerRect, Qt::AlignCenter,
                   m_recordingKey ? tr(" RECORDING: Press any key on keyboard...")
                                  : tr(" Double-click canvas to add  |  Drag nodes to move  |  Esc to exit"));
    }

    // Toast message (e.g. "Keymap Applied")
    if (!m_toastMessage.isEmpty()) {
        int tw = 340;
        int th = 40;
        QRect toastRect((width() - tw) / 2, height() - 70, tw, th);
        p.setPen(QPen(QColor(16, 185, 129), 1.5));
        p.setBrush(QColor(6, 78, 59, 230));
        p.drawRoundedRect(toastRect, 8, 8);

        QFont tFont = p.font();
        tFont.setPointSize(10);
        tFont.setBold(true);
        p.setFont(tFont);
        p.setPen(Qt::white);
        p.drawText(toastRect, Qt::AlignCenter, m_toastMessage);
    }
}

// ---------------------------------------------------------------------------
// Canvas Mouse Events
// ---------------------------------------------------------------------------
void OverlayPanel::mousePressEvent(QMouseEvent *e)
{
    if (m_editMode) {
        if (m_recordingKey && m_selected) {
            if (e->button() == Qt::RightButton) {
                m_selected->setKey("RightButton");
                if (m_selected->buttonType() == OverlayButtonType::Aim) {
                    m_switchKey = "RightButton";
                }
                m_recordingKey = false;
                populatePropsFromButton(m_selected);
                update();
                e->accept();
                return;
            } else if (e->button() == Qt::MiddleButton) {
                m_selected->setKey("MidButton");
                if (m_selected->buttonType() == OverlayButtonType::Aim) {
                    m_switchKey = "MidButton";
                }
                m_recordingKey = false;
                populatePropsFromButton(m_selected);
                update();
                e->accept();
                return;
            }
        }
        if (e->button() == Qt::LeftButton) {
            // Deselect button if clicking on background
            selectButton(nullptr);
            e->accept();
            return;
        }
    }
    QWidget::mousePressEvent(e);
}

void OverlayPanel::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (m_editMode && e->button() == Qt::LeftButton) {
        QRect vArea = currentVideoGeometry();
        if (vArea.contains(e->pos())) {
            double rx = static_cast<double>(e->pos().x() - vArea.left()) / qMax(1, vArea.width());
            double ry = static_cast<double>(e->pos().y() - vArea.top())  / qMax(1, vArea.height());
            auto *btn = createButton(OverlayButtonType::Click, "Key", "Key_J", QPointF(rx, ry));
            selectButton(btn);
            e->accept();
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(e);
}

// ---------------------------------------------------------------------------
// Keyboard Handling
// ---------------------------------------------------------------------------
void OverlayPanel::keyPressEvent(QKeyEvent *e)
{
    if (m_editMode) {
        // Key Recording Mode
        if (m_recordingKey && m_selected) {
            QString qtKeyName;
            quint32 vk = e->nativeVirtualKey();
            if (vk >= 'A' && vk <= 'Z') {
                qtKeyName = QString("Key_%1").arg(QChar(vk));
            } else if (vk >= '0' && vk <= '9') {
                qtKeyName = QString("Key_%1").arg(QChar(vk));
            } else if (vk == 0x20) { // VK_SPACE
                qtKeyName = "Key_Space";
            } else if (vk == 0x10) { // VK_SHIFT
                qtKeyName = "Key_Shift";
            } else if (vk == 0x11) { // VK_CONTROL
                qtKeyName = "Key_Control";
            } else if (vk == 0x12) { // VK_MENU (Alt)
                qtKeyName = "Key_Alt";
            } else if (vk == 0x09) { // VK_TAB
                qtKeyName = "Key_Tab";
            } else if (vk == 0xC0) { // VK_OEM_3 (`~)
                qtKeyName = "Key_QuoteLeft";
            }

            if (qtKeyName.isEmpty()) {
                qtKeyName = OverlayButton::mapArabicOrNumberToLatinKey(QString::number(e->key()));
            }
            if (qtKeyName.isEmpty()) {
                qtKeyName = qtKeyToString(e->key());
            }
            QString mapped = OverlayButton::mapArabicOrNumberToLatinKey(qtKeyName);
            if (!mapped.isEmpty()) {
                qtKeyName = mapped;
            }

            m_selected->setKey(qtKeyName);
            if (m_selected->buttonType() == OverlayButtonType::Aim) {
                m_switchKey = qtKeyName;
            }
            m_recordingKey = false;
            populatePropsFromButton(m_selected);
            update();
            e->accept();
            return;
        }

        if (e->key() == Qt::Key_Escape) {
            setEditMode(false);
            e->accept();
            return;
        }

        if ((e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) && m_selected) {
            onDeleteSelected();
            e->accept();
            return;
        }
    }
    QWidget::keyPressEvent(e);
}

void OverlayPanel::keyReleaseEvent(QKeyEvent *e)
{
    QWidget::keyReleaseEvent(e);
}

// ---------------------------------------------------------------------------
// Key Conversions
// ---------------------------------------------------------------------------
QString OverlayPanel::qtKeyToString(int key)
{
    QString mapped = OverlayButton::mapArabicOrNumberToLatinKey(QString::number(key));
    if (!mapped.isEmpty()) {
        return mapped;
    }

    QMetaEnum meta = QMetaEnum::fromType<Qt::Key>();
    const char *name = meta.valueToKey(key);
    if (name) {
        return QString::fromLatin1(name);
    }
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return QString("Key_%1").arg(QChar('A' + (key - Qt::Key_A)));
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return QString("Key_%1").arg(QChar('0' + (key - Qt::Key_0)));
    }
    return QString("Key_%1").arg(key);
}

int OverlayPanel::stringToQtKey(const QString &keyStr)
{
    if (keyStr.isEmpty()) return Qt::Key_unknown;
    QString norm = keyStr.trimmed();
    if (!norm.startsWith("Key_")) {
        norm = "Key_" + norm;
    }
    QMetaEnum meta = QMetaEnum::fromType<Qt::Key>();
    int val = meta.keyToValue(norm.toUtf8().constData());
    if (val != -1) return val;

    if (norm == "Key_SPACE") return Qt::Key_Space;
    if (norm == "Key_CTRL") return Qt::Key_Control;
    if (norm == "Key_SHIFT") return Qt::Key_Shift;
    if (norm == "Key_ALT") return Qt::Key_Alt;
    if (norm == "Key_TAB") return Qt::Key_Tab;
    if (norm == "Key_ESC") return Qt::Key_Escape;
    if (norm == "Key_ENTER") return Qt::Key_Return;
    if (norm == "Key_~" || norm == "Key_`") return Qt::Key_QuoteLeft;

    return Qt::Key_unknown;
}

QString OverlayPanel::keyToDisplayLabel(const QString &keyStr)
{
    QString k = keyStr.trimmed();
    if (k == "LeftButton" || k == "Left") return "L-CLICK";
    if (k == "RightButton" || k == "Right") return "R-CLICK";
    if (k == "MidButton" || k == "Middle") return "M-CLICK";

    QString mapped = OverlayButton::mapArabicOrNumberToLatinKey(k);
    if (!mapped.isEmpty()) {
        return mapped.mid(4);
    }

    if (k.startsWith("Key_")) k = k.mid(4);
    if (k == "QuoteLeft") return "~";
    if (k == "Space")     return "SPACE";
    if (k == "Return")    return "ENTER";
    if (k == "Shift")     return "SHIFT";
    if (k == "Control")   return "CTRL";
    if (k == "Alt")       return "ALT";
    if (k == "Tab")       return "TAB";
    if (k == "Escape")    return "ESC";
    return k.toUpper();
}

// ---------------------------------------------------------------------------
// Node Selection & Inspector
// ---------------------------------------------------------------------------
void OverlayPanel::selectButton(OverlayButton *btn)
{
    if (m_selected && m_selected != btn) {
        m_selected->setSelected(false);
    }
    m_selected = btn;
    m_recordingKey = false;

    if (m_selected) {
        m_selected->setSelected(true);
        populatePropsFromButton(m_selected);
        m_propsWidget->setEnabled(true);
    } else {
        m_propsWidget->setEnabled(false);
        m_selTypeLabel->setText(tr("No button selected"));
        m_labelEdit->clear();
        m_keyEdit->clear();
        m_recordKeyBtn->setText(tr(" Bind Key"));
        m_coordsLabel->setText("X: -- | Y: --");
    }
    update();
}

void OverlayPanel::populatePropsFromButton(OverlayButton *btn)
{
    if (!btn) return;

    QString typeStr = "Click Node";
    if (btn->buttonType() == OverlayButtonType::DoubleClick) typeStr = " Double Tap Node";
    else if (btn->buttonType() == OverlayButtonType::Joystick)    typeStr = " WASD Movement Wheel";
    else if (btn->buttonType() == OverlayButtonType::Aim)         typeStr = " FPS Aim & Look Node";
    else if (btn->buttonType() == OverlayButtonType::Swipe)       typeStr = " Swipe Gesture Node";

    m_selTypeLabel->setText(typeStr);
    m_labelEdit->blockSignals(true);
    m_keyEdit->blockSignals(true);
    m_sizeSlider->blockSignals(true);

    m_labelEdit->setText(btn->label());
    m_keyEdit->setText(btn->key());
    m_recordKeyBtn->setText(QString(" [%1]").arg(keyToDisplayLabel(btn->key())));
    m_sizeSlider->setValue(static_cast<int>(btn->radiusRatio() * 1000));
    m_coordsLabel->setText(QString("X: %1 | Y: %2")
                           .arg(btn->posRatio().x(), 0, 'f', 2)
                           .arg(btn->posRatio().y(), 0, 'f', 2));

    m_joyGroup->setVisible(btn->buttonType() == OverlayButtonType::Joystick);
    m_aimGroup->setVisible(btn->buttonType() == OverlayButtonType::Aim);
    m_clickGroup->setVisible(btn->buttonType() == OverlayButtonType::Click);

    if (m_opacitySlider) {
        m_opacitySlider->blockSignals(true);
        m_opacitySlider->setValue(static_cast<int>(m_hudOpacity * 100));
        m_opacitySlider->blockSignals(false);
    }

    if (btn->buttonType() == OverlayButtonType::Joystick) {
        m_joyUpEdit->setText(btn->upKey());
        m_joyDownEdit->setText(btn->downKey());
        m_joyLeftEdit->setText(btn->leftKey());
        m_joyRightEdit->setText(btn->rightKey());
    } else if (btn->buttonType() == OverlayButtonType::Aim) {
        m_speedXSlider->setValue(static_cast<int>(btn->speedRatioX() * 10));
        m_speedValLabel->setText(QString("Aim Sensitivity: %1x").arg(btn->speedRatioX(), 0, 'f', 1));
    } else if (btn->buttonType() == OverlayButtonType::Click) {
        m_switchMapCheck->setChecked(btn->switchMap());
    }

    m_labelEdit->blockSignals(false);
    m_keyEdit->blockSignals(false);
    m_sizeSlider->blockSignals(false);
}

void OverlayPanel::onApplyProps()
{
    if (!m_selected) return;

    m_selected->setLabel(m_labelEdit->text());

    QString rawKey = m_keyEdit->text().trimmed();
    if (!rawKey.isEmpty()) {
        QString normKey = OverlayButton::normalizeKeyName(rawKey, m_selected->key());
        m_selected->setKey(normKey);
        if (m_selected->buttonType() == OverlayButtonType::Aim) {
            m_switchKey = normKey;
        }
        m_recordKeyBtn->setText(QString(" [%1]").arg(keyToDisplayLabel(normKey)));
    }

    m_selected->setRadiusRatio(m_sizeSlider->value() / 1000.0f);

    if (m_selected->buttonType() == OverlayButtonType::Joystick) {
        m_selected->setJoystickKeys(
            m_joyUpEdit->text(),
            m_joyDownEdit->text(),
            m_joyLeftEdit->text(),
            m_joyRightEdit->text()
        );
    } else if (m_selected->buttonType() == OverlayButtonType::Aim) {
        float spd = m_speedXSlider->value() / 10.0f;
        m_selected->setSpeedRatios(spd, spd);
        m_speedValLabel->setText(QString("Aim Sensitivity: %1x").arg(spd, 0, 'f', 1));
    } else if (m_selected->buttonType() == OverlayButtonType::Click) {
        m_selected->setSwitchMap(m_switchMapCheck->isChecked());
    }
}

void OverlayPanel::onRecordKeyClicked()
{
    if (!m_selected) return;
    m_recordingKey = true;
    m_recordKeyBtn->setText(tr(" Press Key or Right-Click..."));
    setFocus();
    update();
}

// ---------------------------------------------------------------------------
// Add Node Slots
// ---------------------------------------------------------------------------
void OverlayPanel::onAddClick()
{
    auto *btn = createButton(OverlayButtonType::Click, "Key", "Key_J", QPointF(0.75, 0.7));
    selectButton(btn);
}

void OverlayPanel::onAddDoubleClick()
{
    auto *btn = createButton(OverlayButtonType::DoubleClick, "Rapid", "Key_Q", QPointF(0.25, 0.4));
    selectButton(btn);
}

void OverlayPanel::onAddJoystick()
{
    auto *btn = createButton(OverlayButtonType::Joystick, "Move", "WASD", QPointF(0.2, 0.72));
    btn->setRadiusRatio(0.09f);
    btn->setJoystickKeys("Key_W", "Key_S", "Key_A", "Key_D");
    selectButton(btn);
}

void OverlayPanel::onAddAim()
{
    QString aimKey = m_switchKey.isEmpty() ? "RightButton" : m_switchKey;
    auto *btn = createButton(OverlayButtonType::Aim, "Aim", aimKey, QPointF(0.65, 0.45));
    m_switchKey = aimKey;
    selectButton(btn);
}

void OverlayPanel::onAddSwipe()
{
    auto *btn = createButton(OverlayButtonType::Swipe, "Swipe", "Key_Up", QPointF(0.5, 0.6));
    selectButton(btn);
}

void OverlayPanel::onAddFire()
{
    auto *btn = createButton(OverlayButtonType::Click, "Fire", "LeftButton", QPointF(0.86, 0.72));
    selectButton(btn);
}

void OverlayPanel::onAddScope()
{
    auto *btn = createButton(OverlayButtonType::Click, "Scope", "RightButton", QPointF(0.95, 0.52));
    selectButton(btn);
}

void OverlayPanel::onAddFreeLook()
{
    auto *btn = createButton(OverlayButtonType::Click, "Eye", "Key_Alt", QPointF(0.80, 0.31));
    selectButton(btn);
}

void OverlayPanel::onAddMap()
{
    auto *btn = createButton(OverlayButtonType::Click, "Map", "Key_M", QPointF(0.96, 0.05));
    btn->setSwitchMap(true);
    selectButton(btn);
}

void OverlayPanel::onAddBag()
{
    auto *btn = createButton(OverlayButtonType::Click, "Bag", "Key_Tab", QPointF(0.06, 0.88));
    btn->setSwitchMap(true);
    selectButton(btn);
}

void OverlayPanel::onSetLeftClick()
{
    if (!m_selected) return;
    m_keyEdit->setText("LeftButton");
    m_selected->setKey("LeftButton");
    if (m_selected->label() == "Key" || m_selected->label().isEmpty()) {
        m_labelEdit->setText("Fire");
        m_selected->setLabel("Fire");
    }
    populatePropsFromButton(m_selected);
    onApplyProps();
}

void OverlayPanel::onSetRightClick()
{
    if (!m_selected) return;
    m_keyEdit->setText("RightButton");
    m_selected->setKey("RightButton");
    if (m_selected->label() == "Key" || m_selected->label().isEmpty()) {
        m_labelEdit->setText("Scope");
        m_selected->setLabel("Scope");
    }
    if (m_selected->buttonType() == OverlayButtonType::Aim) {
        m_switchKey = "RightButton";
    }
    populatePropsFromButton(m_selected);
    onApplyProps();
}

void OverlayPanel::onSetMidClick()
{
    if (!m_selected) return;
    m_keyEdit->setText("MidButton");
    m_selected->setKey("MidButton");
    populatePropsFromButton(m_selected);
    onApplyProps();
}

void OverlayPanel::onOpacitySliderChanged(int val)
{
    m_hudOpacity = val / 100.0f;
    for (auto *b : m_buttons) {
        b->setHudOpacity(m_hudOpacity);
    }
    update();
}

// ---------------------------------------------------------------------------
// Node Interaction Slots
// ---------------------------------------------------------------------------
void OverlayPanel::onButtonEditRequested(OverlayButton *btn)
{
    selectButton(btn);
}

void OverlayPanel::onButtonPosChanged(OverlayButton *btn)
{
    if (m_selected == btn && m_coordsLabel) {
        m_coordsLabel->setText(QString("X: %1 | Y: %2")
                               .arg(btn->posRatio().x(), 0, 'f', 2)
                               .arg(btn->posRatio().y(), 0, 'f', 2));
    }
}

void OverlayPanel::onButtonDeleteRequested(OverlayButton *btn)
{
    if (!btn) return;
    if (m_selected == btn) {
        selectButton(nullptr);
    }
    m_buttons.removeAll(btn);
    btn->deleteLater();
    update();
}

void OverlayPanel::onButtonDuplicateRequested(OverlayButton *btn)
{
    if (!btn) return;
    QPointF newPos(qMin(0.95, btn->posRatio().x() + 0.05),
                   qMin(0.95, btn->posRatio().y() + 0.05));
    auto *dup = createButton(btn->buttonType(), btn->label() + "_copy", btn->key(), newPos);
    dup->setRadiusRatio(btn->radiusRatio());
    dup->setJoystickKeys(btn->upKey(), btn->downKey(), btn->leftKey(), btn->rightKey());
    dup->setSpeedRatios(btn->speedRatioX(), btn->speedRatioY());
    dup->setSwitchMap(btn->switchMap());
    selectButton(dup);
}

void OverlayPanel::onDeleteSelected()
{
    if (m_selected) {
        onButtonDeleteRequested(m_selected);
    }
}

void OverlayPanel::onDuplicateSelected()
{
    if (m_selected) {
        onButtonDuplicateRequested(m_selected);
    }
}

// ---------------------------------------------------------------------------
// Preset Profile Management
// ---------------------------------------------------------------------------
void OverlayPanel::refreshProfileList()
{
    m_presetCombo->blockSignals(true);
    m_presetCombo->clear();
    m_presetCombo->addItem(tr("-- Select Preset --"));

    QSet<QString> names;
    const QList<QDir> dirs = { QDir(userKeymapDirectory()), QDir(defaultKeymapDirectory()) };
    for (const auto &d : dirs) {
        if (!d.exists()) continue;
        const auto list = d.entryInfoList(QStringList() << "*.json", QDir::Files);
        for (const auto &fi : list) {
            if (!names.contains(fi.fileName())) {
                names.insert(fi.fileName());
                m_presetCombo->addItem(fi.fileName(), fi.absoluteFilePath());
            }
        }
    }
    m_presetCombo->blockSignals(false);
}

void OverlayPanel::onProfilePresetSelected(int index)
{
    if (index <= 0) return;
    QString path = m_presetCombo->itemData(index).toString();
    if (!path.isEmpty()) {
        loadLayout(path);
        QFileInfo fi(path);
        m_profileEdit->setText(fi.baseName());
        m_currentProfileName = fi.baseName();
    }
}

// ---------------------------------------------------------------------------
// Serialization (Standard QtScrcpy JSON + Visual Metadata)
// ---------------------------------------------------------------------------
bool OverlayPanel::saveLayout()
{
    m_currentProfileName = m_profileEdit->text().trimmed();
    if (m_currentProfileName.isEmpty()) {
        m_currentProfileName = "custom_keymap";
    }

    QJsonObject root;
    root["switchKey"] = m_switchKey.isEmpty() ? "Key_QuoteLeft" : m_switchKey;

    QJsonArray keyMapNodes;
    QJsonObject mouseMoveMap;
    bool hasAim = false;

    // Build standard QtScrcpy keyMapNodes
    for (auto *btn : m_buttons) {
        if (!btn) continue;
        if (btn->buttonType() == OverlayButtonType::Aim) {
            hasAim = true;
            mouseMoveMap["speedRatioX"] = static_cast<double>(btn->speedRatioX());
            mouseMoveMap["speedRatioY"] = static_cast<double>(btn->speedRatioY());
            mouseMoveMap["speedRatio"]  = 10;
            QJsonObject startPos;
            startPos["x"] = btn->posRatio().x();
            startPos["y"] = btn->posRatio().y();
            mouseMoveMap["startPos"] = startPos;
            if (!btn->key().isEmpty()) {
                m_switchKey = btn->key();
            }
        } else {
            keyMapNodes.append(btn->toKeyMapNode());
        }
    }

    root["switchKey"] = m_switchKey.isEmpty() ? "Key_QuoteLeft" : m_switchKey;

    if (hasAim) {
        root["mouseMoveMap"] = mouseMoveMap;
    }
    root["keyMapNodes"] = keyMapNodes;

    // Visual configuration for pixel-perfect reload
    QJsonArray visualArr;
    for (auto *btn : m_buttons) {
        if (btn) visualArr.append(btn->toJson());
    }
    root["_visualButtons"] = visualArr;

    QString savePath = userKeymapDirectory() + "/" + m_currentProfileName + ".json";
    QFile f(savePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();

    m_currentJsonPath = savePath;
    emit layoutChanged();
    return true;
}

void OverlayPanel::applyToDevice()
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }

    // Build standard compact script JSON for QtScrcpyCore
    QJsonObject root;
    QJsonArray keyMapNodes;
    QJsonObject mouseMoveMap;
    bool hasAim = false;

    for (auto *btn : m_buttons) {
        if (!btn) continue;
        if (btn->buttonType() == OverlayButtonType::Aim) {
            hasAim = true;
            mouseMoveMap["speedRatioX"] = static_cast<double>(btn->speedRatioX());
            mouseMoveMap["speedRatioY"] = static_cast<double>(btn->speedRatioY());
            mouseMoveMap["speedRatio"]  = 10;
            QJsonObject startPos;
            startPos["x"] = btn->posRatio().x();
            startPos["y"] = btn->posRatio().y();
            mouseMoveMap["startPos"] = startPos;
            if (!btn->key().isEmpty()) {
                m_switchKey = btn->key();
            }
        } else {
            keyMapNodes.append(btn->toKeyMapNode());
        }
    }

    root["switchKey"] = m_switchKey.isEmpty() ? "Key_QuoteLeft" : m_switchKey;

    if (hasAim) {
        root["mouseMoveMap"] = mouseMoveMap;
    }
    root["keyMapNodes"] = keyMapNodes;

    QString script = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
    device->updateScript(script);

    // CRITICAL: If custom keymap is not currently active on device, activate it immediately!
    if (!device->isCurrentCustomKeymap()) {
        QRect vArea = currentVideoGeometry();
        QSize widgetSize = vArea.size();
        QSize frameSize = widgetSize;
        if (VideoForm *vf = qobject_cast<VideoForm *>(parentWidget())) {
            frameSize = vf->frameSize();
        }

        if (m_switchKey == "RightButton" || m_switchKey == "Right") {
            QPointF localPt = vArea.center();
            QPointF globalPt = mapToGlobal(vArea.center());
            QMouseEvent pressEv(QEvent::MouseButtonPress, localPt, globalPt, Qt::RightButton, Qt::MouseButtons(Qt::RightButton), Qt::NoModifier);
            QMouseEvent releaseEv(QEvent::MouseButtonRelease, localPt, globalPt, Qt::RightButton, Qt::NoButton, Qt::NoModifier);
            emit device->mouseEvent(&pressEv, frameSize, widgetSize);
            emit device->mouseEvent(&releaseEv, frameSize, widgetSize);
        } else {
            int switchQtKey = stringToQtKey(m_switchKey.isEmpty() ? "Key_QuoteLeft" : m_switchKey);
            if (switchQtKey == Qt::Key_unknown) {
                switchQtKey = Qt::Key_QuoteLeft;
            }
            QKeyEvent pressEv(QEvent::KeyPress, switchQtKey, Qt::NoModifier);
            QKeyEvent releaseEv(QEvent::KeyRelease, switchQtKey, Qt::NoModifier);
            emit device->keyEvent(&pressEv, frameSize, widgetSize);
            emit device->keyEvent(&releaseEv, frameSize, widgetSize);
        }
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

    // Clear existing buttons
    for (auto *b : m_buttons) {
        b->deleteLater();
    }
    m_buttons.clear();
    selectButton(nullptr);

    if (root.contains("switchKey")) {
        m_switchKey = root["switchKey"].toString("Key_QuoteLeft");
    }

    // Prefer visual companion data if present
    if (root.contains("_visualButtons")) {
        QJsonArray arr = root["_visualButtons"].toArray();
        for (const auto &val : arr) {
            OverlayButton *b = OverlayButton::fromJson(val.toObject(), this);
            b->setEditMode(m_editMode);
            b->reposition(currentVideoGeometry());
            connect(b, &OverlayButton::editRequested,      this, &OverlayPanel::onButtonEditRequested);
            connect(b, &OverlayButton::posRatioChanged,    this, &OverlayPanel::onButtonPosChanged);
            connect(b, &OverlayButton::deleteRequested,    this, &OverlayPanel::onButtonDeleteRequested);
            connect(b, &OverlayButton::duplicateRequested, this, &OverlayPanel::onButtonDuplicateRequested);
            m_buttons.append(b);
            b->show();
        }
    }
    // Backward compatibility with legacy custom layout
    else if (root.contains("buttons")) {
        QJsonArray arr = root["buttons"].toArray();
        for (const auto &val : arr) {
            OverlayButton *b = OverlayButton::fromJson(val.toObject(), this);
            b->setEditMode(m_editMode);
            b->reposition(currentVideoGeometry());
            connect(b, &OverlayButton::editRequested,      this, &OverlayPanel::onButtonEditRequested);
            connect(b, &OverlayButton::posRatioChanged,    this, &OverlayPanel::onButtonPosChanged);
            connect(b, &OverlayButton::deleteRequested,    this, &OverlayPanel::onButtonDeleteRequested);
            connect(b, &OverlayButton::duplicateRequested, this, &OverlayPanel::onButtonDuplicateRequested);
            m_buttons.append(b);
            b->show();
        }
    }
    // Load standard QtScrcpy keyMapNodes
    else if (root.contains("keyMapNodes")) {
        if (root.contains("mouseMoveMap")) {
            QJsonObject mm = root["mouseMoveMap"].toObject();
            QJsonObject sp = mm["startPos"].toObject();
            double sx = sp["x"].toDouble(0.65);
            double sy = sp["y"].toDouble(0.45);
            auto *aimBtn = createButton(OverlayButtonType::Aim, "Aim", m_switchKey, QPointF(sx, sy));
            aimBtn->setSpeedRatios(
                static_cast<float>(mm["speedRatioX"].toDouble(2.5)),
                static_cast<float>(mm["speedRatioY"].toDouble(2.5))
            );
        }

        QJsonArray nodes = root["keyMapNodes"].toArray();
        for (const auto &val : nodes) {
            QJsonObject node = val.toObject();
            QString type = node["type"].toString();
            QString comment = node["comment"].toString();

            if (type == "KMT_CLICK") {
                QJsonObject pos = node["pos"].toObject();
                auto *btn = createButton(OverlayButtonType::Click, comment,
                                         node["key"].toString(),
                                         QPointF(pos["x"].toDouble(), pos["y"].toDouble()));
                btn->setSwitchMap(node["switchMap"].toBool(false));

            } else if (type == "KMT_CLICK_TWICE") {
                QJsonObject pos = node["pos"].toObject();
                createButton(OverlayButtonType::DoubleClick, comment,
                             node["key"].toString(),
                             QPointF(pos["x"].toDouble(), pos["y"].toDouble()));

            } else if (type == "KMT_STEER_WHEEL") {
                QJsonObject cp = node["centerPos"].toObject();
                auto *joy = createButton(OverlayButtonType::Joystick, comment, "WASD",
                                         QPointF(cp["x"].toDouble(), cp["y"].toDouble()));
                joy->setJoystickKeys(
                    node["upKey"].toString("Key_W"),
                    node["downKey"].toString("Key_S"),
                    node["leftKey"].toString("Key_A"),
                    node["rightKey"].toString("Key_D")
                );

            } else if (type == "KMT_DRAG") {
                QJsonObject sp = node["startPos"].toObject();
                createButton(OverlayButtonType::Swipe, comment,
                             node["key"].toString(),
                             QPointF(sp["x"].toDouble(), sp["y"].toDouble()));
            }
        }
    }

    emit layoutChanged();
    update();
}

// ---------------------------------------------------------------------------
// Action Bar Slots
// ---------------------------------------------------------------------------
void OverlayPanel::onSaveAndApply()
{
    saveLayout();
    applyToDevice();

    // Switch to Play Mode with HUD enabled
    setEditMode(false);
    setOverlayVisible(true);

    // Toast notification
    m_toastMessage = tr(" Keymap Applied & Active! Controls are working.");
    update();

    QTimer::singleShot(3500, this, [this]() {
        m_toastMessage.clear();
        update();
    });
}

void OverlayPanel::onToggleHUD()
{
    setOverlayVisible(!m_overlayOn);
}

void OverlayPanel::onExportKeymap()
{
    saveLayout();
    QString srcPath = userKeymapDirectory() + "/" + m_currentProfileName + ".json";
    QString destPath = QFileDialog::getSaveFileName(this, tr("Export Keymap JSON"),
                                                    m_currentProfileName + ".json",
                                                    tr("JSON Files (*.json)"));
    if (!destPath.isEmpty()) {
        if (QFile::exists(destPath)) {
            QFile::remove(destPath);
        }
        if (QFile::copy(srcPath, destPath)) {
            m_toastMessage = tr(" Keymap exported successfully!");
        } else {
            m_toastMessage = tr(" Failed to export keymap!");
        }
        update();
        QTimer::singleShot(3500, this, [this]() {
            m_toastMessage.clear();
            update();
        });
    }
}

void OverlayPanel::onCloseEdit()
{
    setEditMode(false);
}
