#ifndef OVERLAYBUTTON_H
#define OVERLAYBUTTON_H

#include <QWidget>
#include <QString>
#include <QPointF>
#include <QRect>
#include <QJsonObject>

/**
 * @brief Types of overlay buttons supported in the Visual Keymap system
 */
enum class OverlayButtonType {
    Click,        // Single tap (KMT_CLICK)
    DoubleClick,  // Double tap (KMT_CLICK_TWICE)
    Joystick,     // Virtual analog stick / WASD (KMT_STEER_WHEEL)
    Aim,          // FPS mouse camera aim (mouseMoveMap)
    Swipe         // Swipe gesture (KMT_DRAG)
};

/**
 * @brief A single draggable/clickable overlay button drawn on top of VideoForm.
 *
 * In Play Mode  : transparent to all mouse events, drawn as a clean semi-transparent HUD.
 * In Edit Mode  : handles drag/drop, selection, right-click menu, and property editing.
 *
 * Positions are stored as ratios (0.0–1.0) relative to the phone video rendering area.
 */
class OverlayButton : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayButton(OverlayButtonType type,
                           const QString    &label,
                           const QString    &key,
                           QPointF           posRatio,
                           QWidget          *parent = nullptr);

    // --- Serialisation ---
    QJsonObject toJson() const;
    static OverlayButton *fromJson(const QJsonObject &obj, QWidget *parent);

    // Export as native QtScrcpy keyMapNode
    QJsonObject toKeyMapNode() const;

    // --- Accessors ---
    OverlayButtonType buttonType() const { return m_type; }
    void              setButtonType(OverlayButtonType t);

    QString           label()      const { return m_label; }
    QString           key()        const { return m_key; }
    QString           displayKey() const;
    QPointF           posRatio()   const { return m_posRatio; }
    float             radiusRatio()const { return m_radiusRatio; }

    void setLabel(const QString &l);
    void setKey(const QString &k);
    void setPosRatio(const QPointF &p);
    void setRadiusRatio(float r);

    // Joystick specific keys
    QString upKey()    const { return m_upKey; }
    QString downKey()  const { return m_downKey; }
    QString leftKey()  const { return m_leftKey; }
    QString rightKey() const { return m_rightKey; }
    void setJoystickKeys(const QString &u, const QString &d, const QString &l, const QString &r);

    // Aim sensitivity
    float speedRatioX() const { return m_speedRatioX; }
    float speedRatioY() const { return m_speedRatioY; }
    void  setSpeedRatios(float x, float y);

    // Click mouse release (switchMap)
    bool switchMap() const { return m_switchMap; }
    void setSwitchMap(bool sm);

    // HUD Opacity
    float hudOpacity() const { return m_hudOpacity; }
    void  setHudOpacity(float op);

    // Static helper for key mapping & arabic translation
    static QString mapArabicOrNumberToLatinKey(const QString &raw);
    static QString normalizeKeyName(const QString &raw, const QString &defaultKey);

    // --- Mode & Selection ---
    void setEditMode(bool edit);
    bool isEditMode() const { return m_editMode; }

    void setSelected(bool sel);
    bool isSelected() const { return m_selected; }

    // Reposition relative to a specific sub-rect (the phone video area) or parent widget
    void reposition(const QRect &videoArea);
    void repositionFromRatio();

signals:
    void editRequested(OverlayButton *self);
    void deleteRequested(OverlayButton *self);
    void duplicateRequested(OverlayButton *self);
    void posRatioChanged(OverlayButton *self);

protected:
    void paintEvent(QPaintEvent *event)          override;
    void mousePressEvent(QMouseEvent *e)         override;
    void mouseMoveEvent(QMouseEvent *e)          override;
    void mouseReleaseEvent(QMouseEvent *e)       override;
    void mouseDoubleClickEvent(QMouseEvent *e)   override;
    void contextMenuEvent(QContextMenuEvent *e)  override;

private:
    int baseRadius() const;
    QRect currentTargetArea() const;

    OverlayButtonType m_type;
    QString           m_label;
    QString           m_key;          // e.g. "Key_J", "Key_Space"
    QPointF           m_posRatio;     // normalized (0.0 to 1.0)
    float             m_radiusRatio  = 0.055f;

    // Joystick parameters
    QString           m_upKey    = "Key_W";
    QString           m_downKey  = "Key_S";
    QString           m_leftKey  = "Key_A";
    QString           m_rightKey = "Key_D";

    // Aim parameters
    float             m_speedRatioX = 2.5f;
    float             m_speedRatioY = 2.5f;

    // Click parameters
    bool              m_switchMap = false;
    float             m_hudOpacity = 0.85f;

    // UI State
    bool              m_editMode    = false;
    bool              m_selected    = false;
    bool              m_dragging    = false;
    QPoint            m_dragOffset;
    QRect             m_lastVideoArea;
};

#endif // OVERLAYBUTTON_H
