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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QRect>

#include "overlaybutton.h"

/**
 * @brief Professional Embedded Overlay and Keymap Studio for VideoForm.
 *
 * In Edit Mode:
 *  - Displays transparent tactical grid over phone video.
 *  - Right-side docking panel with Add controls, Live Key Recorder, and Property Inspector.
 *  - Real-time drag & drop with boundary clamping.
 *  - Double-click on canvas creates a new button instantly.
 *
 * In Play Mode:
 *  - Overlay buttons remain visible as a clean semi-transparent gaming HUD.
 *  - WA_TransparentForMouseEvents is TRUE so all mouse clicks and movements
 *    pass directly through to VideoForm and the phone.
 *  - Focus is released to VideoForm so keyboard events are dispatched immediately.
 *  - Save & Apply compiles standard QtScrcpy JSON, applies it to the device via
 *    IDevice::updateScript(), and activates custom keymap mode automatically!
 */
class OverlayPanel : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayPanel(const QString &serial, QWidget *parent = nullptr);
    ~OverlayPanel();

    // Called on parent resize to keep overlay and buttons pixel-perfect
    void onParentResized();

    // Edit vs Play mode
    void setEditMode(bool edit);
    bool isEditMode() const { return m_editMode; }

    // Visual HUD display toggle
    void setOverlayVisible(bool v);
    bool isOverlayVisible() const { return m_overlayOn; }

    // Load & Save
    void loadLayout(const QString &jsonPath);
    bool saveLayout();
    void applyToDevice();
    QString switchKey() const { return m_switchKey; }

    QRect currentVideoGeometry() const;

signals:
    void layoutChanged();

protected:
    void resizeEvent(QResizeEvent *e)       override;
    void paintEvent(QPaintEvent  *e)        override;
    void mousePressEvent(QMouseEvent *e)    override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e)        override;
    void keyReleaseEvent(QKeyEvent *e)      override;

private slots:
    void onAddClick();
    void onAddDoubleClick();
    void onAddJoystick();
    void onAddAim();
    void onAddSwipe();
    void onAddFire();
    void onAddScope();
    void onAddFreeLook();
    void onAddMap();
    void onAddBag();
    void onSetLeftClick();
    void onSetRightClick();
    void onSetMidClick();
    void onOpacitySliderChanged(int val);
    void onSaveAndApply();
    void onExportKeymap();
    void onToggleHUD();
    void onCloseEdit();
    void onDeleteSelected();
    void onDuplicateSelected();
    void onButtonEditRequested(OverlayButton *btn);
    void onButtonPosChanged(OverlayButton *btn);
    void onButtonDeleteRequested(OverlayButton *btn);
    void onButtonDuplicateRequested(OverlayButton *btn);
    void onApplyProps();
    void onRecordKeyClicked();
    void onProfilePresetSelected(int index);

private:
    void buildSidePanel();
    void updateSidePanelGeometry();
    void selectButton(OverlayButton *btn);
    void refreshProfileList();
    void populatePropsFromButton(OverlayButton *btn);
    QString userKeymapDirectory() const;
    QString defaultKeymapDirectory() const;

    OverlayButton *createButton(OverlayButtonType type,
                                const QString &label,
                                const QString &key,
                                QPointF posRatio);

    // Helpers for key conversions
    static QString qtKeyToString(int key);
    static int stringToQtKey(const QString &keyStr);
    static QString keyToDisplayLabel(const QString &keyStr);

    // State
    QString m_serial;
    QString m_currentJsonPath;
    QString m_currentProfileName = "custom_keymap";
    QString m_switchKey = "Key_QuoteLeft";
    bool    m_editMode    = false;
    bool    m_overlayOn   = true;
    bool    m_recordingKey = false;
    QString m_toastMessage;
    int     m_toastTimer  = 0;

    // Buttons
    QList<OverlayButton *> m_buttons;
    OverlayButton         *m_selected = nullptr;

    // UI Widgets in Side Panel
    QFrame      *m_sidePanel      = nullptr;
    QLineEdit   *m_profileEdit    = nullptr;
    QComboBox   *m_presetCombo    = nullptr;

    // Inspector widgets
    QWidget     *m_propsWidget    = nullptr;
    QLabel      *m_selTypeLabel   = nullptr;
    QLineEdit   *m_labelEdit      = nullptr;
    QPushButton *m_recordKeyBtn   = nullptr;
    QLineEdit   *m_keyEdit        = nullptr;
    QSlider     *m_sizeSlider     = nullptr;
    QLabel      *m_coordsLabel    = nullptr;

    // Joystick specific controls
    QWidget     *m_joyGroup       = nullptr;
    QLineEdit   *m_joyUpEdit      = nullptr;
    QLineEdit   *m_joyDownEdit    = nullptr;
    QLineEdit   *m_joyLeftEdit    = nullptr;
    QLineEdit   *m_joyRightEdit   = nullptr;

    // Aim specific controls
    QWidget     *m_aimGroup       = nullptr;
    QSlider     *m_speedXSlider   = nullptr;
    QSlider     *m_speedYSlider   = nullptr;
    QLabel      *m_speedValLabel  = nullptr;

    // Click specific controls
    QWidget     *m_clickGroup     = nullptr;
    QCheckBox   *m_switchMapCheck = nullptr;

    // HUD Opacity
    QSlider     *m_opacitySlider  = nullptr;
    float        m_hudOpacity     = 0.85f;

    // Action buttons
    QPushButton *m_deleteBtn      = nullptr;
    QPushButton *m_saveBtn        = nullptr;
    QPushButton *m_hudToggleBtn   = nullptr;
    QPushButton *m_closeBtn       = nullptr;
};

#endif // OVERLAYPANEL_H
