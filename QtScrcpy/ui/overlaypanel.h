#ifndef OVERLAYPANEL_H
#define OVERLAYPANEL_H

#include <QWidget>
#include <QList>
#include <QString>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QSlider>
#include <QCheckBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QRect>
#include <QTimer>
#include <QListWidget>

#include "overlaybutton.h"

/**
 * @brief Professional Embedded Overlay and Keymap Studio for VideoForm.
 *
 * Features:
 *  - Full TC Gaming / WASD+ button set (Fire, Scope, WASD, Aim, Macro, Spray, etc.)
 *  - Tabbed sidebar: Basic Controls | Advanced | Macros | Settings
 *  - Live Properties Inspector per selected button
 *  - Import / Export JSON profiles
 *  - Clear All / Duplicate / Delete
 *  - HUD opacity control
 */
#include "../uibase/magneticwidget.h"
class OverlayPanel : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayPanel(const QString &serial, QWidget *parent = nullptr);
    ~OverlayPanel();

    void onParentResized();

    void setEditMode(bool edit);
    bool isEditMode() const { return m_editMode; }

    void setOverlayVisible(bool v);
    bool isOverlayVisible() const { return m_overlayOn; }

    void loadLayout(const QString &jsonPath);
    bool saveLayout();
    void applyToDevice();
    QString switchKey() const { return m_switchKey; }

    QRect currentVideoGeometry() const;

signals:
    void layoutChanged();

protected:
    void resizeEvent(QResizeEvent *e)           override;
    void paintEvent(QPaintEvent  *e)            override;
    void mousePressEvent(QMouseEvent *e)        override;
    void mouseDoubleClickEvent(QMouseEvent *e)  override;
    void keyPressEvent(QKeyEvent *e)            override;
    void keyReleaseEvent(QKeyEvent *e)          override;

private slots:
    // --- Add buttons (Basic tab) ---
    void onAddClick();
    void onAddDoubleClick();
    void onAddRightClick();
    void onAddMiddleClick();
    void onAddJoystick();
    void onAddAim();
    void onAddSwipe();
    void onAddFreeLook();

    // --- Add buttons (Game Controls tab) ---
    void onAddFire();
    void onAddScope();
    void onAddJump();
    void onAddProne();
    void onAddGrenade();
    void onAddMap();
    void onAddBag();
    void onAddVehicle();
    void onAddSkill();

    // --- Add buttons (Advanced tab) ---
    void onAddMacro();
    void onAddSpray();

    // --- Old compat slots ---
    void onSetLeftClick();
    void onSetRightClick();
    void onSetMidClick();

    // --- Properties ---
    void onOpacitySliderChanged(int val);
    void onSaveAndApply();
    void onExportKeymap();
    void onImportKeymap();
    void onClearAll();
    void onToggleHUD();
    void onCloseEdit();
    void onDeleteSelected();
    void onDuplicateSelected();
    void onApplyProps();
    void onRecordKeyClicked();
    void onProfilePresetSelected(int index);
    void onMacroStepAdd();
    void onMacroStepRemove();

    // --- Button callbacks ---
    void onButtonEditRequested(OverlayButton *btn);
    void onButtonPosChanged(OverlayButton *btn);
    void onButtonDeleteRequested(OverlayButton *btn);
    void onButtonDuplicateRequested(OverlayButton *btn);
    void onButtonSelected(OverlayButton *btn);

private:
    void buildSidePanel();
    void buildBasicTab(QWidget *tab);
    void buildGameTab(QWidget *tab);
    void buildAdvancedTab(QWidget *tab);
    void buildPropsPanel(QVBoxLayout *layout);
    void buildSettingsTab(QWidget *tab);

    void updateSidePanelGeometry();
    void selectButton(OverlayButton *btn);
    void refreshProfileList();
    void populatePropsFromButton(OverlayButton *btn);
    void clearPropsPanel();
    void showPropsForType(OverlayButtonType type);
    QString userKeymapDirectory() const;
    QString defaultKeymapDirectory() const;

    OverlayButton *createButton(OverlayButtonType type,
                                const QString &label,
                                const QString &key,
                                QPointF posRatio);

    static QString qtKeyToString(int key);
    static int stringToQtKey(const QString &keyStr);
    static QString keyToDisplayLabel(const QString &keyStr);

    // ---- State ----
    QString m_serial;
    QString m_currentJsonPath;
    QString m_currentProfileName = "custom_keymap";
    QString m_switchKey          = "Key_QuoteLeft";
    bool    m_editMode           = false;
    bool    m_overlayOn          = true;
    bool    m_recordingKey       = false;
    QString m_toastMessage;
    int     m_toastTimer         = 0;

    QList<OverlayButton *> m_buttons;
    OverlayButton         *m_selected = nullptr;

    // ---- Side Panel ----
    MagneticWidget *m_sidePanel = nullptr;
    QTabWidget *m_tabs         = nullptr;
    QComboBox  *m_presetCombo  = nullptr;
    QLineEdit  *m_profileEdit  = nullptr;

    // ---- Properties Inspector ----
    QWidget    *m_propsWidget    = nullptr;
    QLabel     *m_selTypeLabel   = nullptr;
    QLineEdit  *m_labelEdit      = nullptr;
    QPushButton*m_recordKeyBtn   = nullptr;
    QLineEdit  *m_keyEdit        = nullptr;
    QSlider    *m_sizeSlider     = nullptr;
    QLabel     *m_coordsLabel    = nullptr;

    // Joystick props
    QWidget    *m_joyGroup       = nullptr;
    QLineEdit  *m_joyUpEdit      = nullptr;
    QLineEdit  *m_joyDownEdit    = nullptr;
    QLineEdit  *m_joyLeftEdit    = nullptr;
    QLineEdit  *m_joyRightEdit   = nullptr;

    // Aim props
    QWidget    *m_aimGroup       = nullptr;
    QSlider    *m_speedXSlider   = nullptr;
    QSlider    *m_speedYSlider   = nullptr;
    QLabel     *m_speedValLabel  = nullptr;

    // Click props
    QWidget    *m_clickGroup     = nullptr;
    QCheckBox  *m_switchMapCheck = nullptr;

    // Spray props
    QWidget    *m_sprayGroup     = nullptr;
    QSpinBox   *m_sprayInterval  = nullptr;

    // Macro props
    QWidget      *m_macroGroup   = nullptr;
    QListWidget  *m_macroList    = nullptr;
    QLineEdit    *m_macroKeyEdit = nullptr;
    QSpinBox     *m_macroDelay   = nullptr;

    // Swipe end
    QWidget    *m_swipeGroup     = nullptr;
    QLineEdit  *m_swipeEndXEdit  = nullptr;
    QLineEdit  *m_swipeEndYEdit  = nullptr;

    // HUD
    QSlider    *m_opacitySlider  = nullptr;
    float       m_hudOpacity     = 0.85f;

    // Action buttons
    QPushButton *m_deleteBtn     = nullptr;
    QPushButton *m_saveBtn       = nullptr;
    QPushButton *m_hudToggleBtn  = nullptr;
    QPushButton *m_closeBtn      = nullptr;
    QPushButton *m_importBtn     = nullptr;
    QPushButton *m_clearAllBtn   = nullptr;
};

#endif // OVERLAYPANEL_H
