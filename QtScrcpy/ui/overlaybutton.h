#ifndef OVERLAYBUTTON_H
#define OVERLAYBUTTON_H

#include <QWidget>
#include <QString>
#include <QPointF>
#include <QRect>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QColor>

enum class OverlayButtonType {
    Click,
    DoubleClick,
    RightClick,
    MiddleClick,
    Joystick,
    Aim,
    Swipe,
    FreeLook,
    Macro,
    Spray,
    Skill,
    Map,
    Bag,
    Scope,
    Grenade,
    Vehicle,
    Prone,
    Jump,
};

struct MacroStep {
    QString key;
    int     delayMs = 50;
    bool    press   = true;
};

class OverlayButton : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayButton(OverlayButtonType type,
                           const QString    &label,
                           const QString    &key,
                           QPointF           posRatio,
                           QWidget          *parent = nullptr);

    QJsonObject toJson() const;
    static OverlayButton *fromJson(const QJsonObject &obj, QWidget *parent);
    QJsonObject toKeyMapNode() const;

    OverlayButtonType buttonType() const { return m_type; }
    void              setButtonType(OverlayButtonType t);

    QString  label()       const { return m_label; }
    QString  key()         const { return m_key; }
    QString  displayKey()  const;
    QPointF  posRatio()    const { return m_posRatio; }
    float    radiusRatio() const { return m_radiusRatio; }

    void setLabel(const QString &l);
    void setKey(const QString &k);
    void setPosRatio(const QPointF &p);
    void setRadiusRatio(float r);

    QString upKey()    const { return m_upKey; }
    QString downKey()  const { return m_downKey; }
    QString leftKey()  const { return m_leftKey; }
    QString rightKey() const { return m_rightKey; }
    void setJoystickKeys(const QString &u, const QString &d, const QString &l, const QString &r);

    float speedRatioX() const { return m_speedRatioX; }
    float speedRatioY() const { return m_speedRatioY; }
    void  setSpeedRatios(float x, float y);

    bool switchMap() const { return m_switchMap; }
    void setSwitchMap(bool sm);

    float hudOpacity() const { return m_hudOpacity; }
    void  setHudOpacity(float op);

    QList<MacroStep> macroSteps() const { return m_macroSteps; }
    void setMacroSteps(const QList<MacroStep> &steps);
    void addMacroStep(const MacroStep &step);
    void clearMacroSteps();

    int  sprayIntervalMs() const { return m_sprayIntervalMs; }
    void setSprayIntervalMs(int ms);

    QPointF swipeEndRatio() const { return m_swipeEndRatio; }
    void    setSwipeEndRatio(const QPointF &p);

    static QString mapArabicOrNumberToLatinKey(const QString &raw);
    static QString normalizeKeyName(const QString &raw, const QString &defaultKey);

    void setEditMode(bool edit);
    bool isEditMode() const { return m_editMode; }

    void setSelected(bool sel);
    bool isSelected() const { return m_selected; }

    void reposition(const QRect &videoArea);
    void repositionFromRatio();

    static QColor  typeColor(OverlayButtonType t);
    static QString typeName(OverlayButtonType t);
    static QString typeIcon(OverlayButtonType t);

signals:
    void editRequested(OverlayButton *self);
    void deleteRequested(OverlayButton *self);
    void duplicateRequested(OverlayButton *self);
    void posRatioChanged(OverlayButton *self);

protected:
    void paintEvent(QPaintEvent *event)         override;
    void mousePressEvent(QMouseEvent *e)        override;
    void mouseMoveEvent(QMouseEvent *e)         override;
    void mouseReleaseEvent(QMouseEvent *e)      override;
    void mouseDoubleClickEvent(QMouseEvent *e)  override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    int   baseRadius() const;
    QRect currentTargetArea() const;

    OverlayButtonType m_type;
    QString           m_label;
    QString           m_key;
    QPointF           m_posRatio;
    float             m_radiusRatio     = 0.055f;
    QString           m_upKey           = "Key_W";
    QString           m_downKey         = "Key_S";
    QString           m_leftKey         = "Key_A";
    QString           m_rightKey        = "Key_D";
    float             m_speedRatioX     = 2.5f;
    float             m_speedRatioY     = 2.5f;
    bool              m_switchMap       = false;
    float             m_hudOpacity      = 0.85f;
    QList<MacroStep>  m_macroSteps;
    int               m_sprayIntervalMs = 80;
    QPointF           m_swipeEndRatio   = QPointF(0.5f, 0.3f);
    bool              m_editMode        = false;
    bool              m_selected        = false;
    bool              m_dragging        = false;
    QPoint            m_dragOffset;
    QRect             m_lastVideoArea;
};

#endif // OVERLAYBUTTON_H