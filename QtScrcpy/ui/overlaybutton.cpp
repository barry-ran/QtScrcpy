#include "overlaybutton.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QJsonArray>
#include <QApplication>
#include <QScreen>

// ---------------------------------------------------------------------------
// Static helpers: color, name, icon per type
// ---------------------------------------------------------------------------
QColor OverlayButton::typeColor(OverlayButtonType t)
{
    switch (t) {
    case OverlayButtonType::Click:       return QColor("#e74c3c");  // red
    case OverlayButtonType::DoubleClick: return QColor("#e67e22");  // orange
    case OverlayButtonType::RightClick:  return QColor("#16a085");  // teal
    case OverlayButtonType::MiddleClick: return QColor("#8e44ad");  // purple
    case OverlayButtonType::Joystick:    return QColor("#2980b9");  // blue
    case OverlayButtonType::Aim:         return QColor("#c0392b");  // dark red
    case OverlayButtonType::Swipe:       return QColor("#d35400");  // dark orange
    case OverlayButtonType::FreeLook:    return QColor("#27ae60");  // green
    case OverlayButtonType::Macro:       return QColor("#f39c12");  // yellow
    case OverlayButtonType::Spray:       return QColor("#c0392b");  // fire red
    case OverlayButtonType::Skill:       return QColor("#9b59b6");  // purple
    case OverlayButtonType::Map:         return QColor("#1abc9c");  // mint
    case OverlayButtonType::Bag:         return QColor("#7f8c8d");  // gray
    case OverlayButtonType::Scope:       return QColor("#2c3e50");  // dark
    case OverlayButtonType::Grenade:     return QColor("#e74c3c");  // red
    case OverlayButtonType::Vehicle:     return QColor("#3498db");  // sky blue
    case OverlayButtonType::Prone:       return QColor("#795548");  // brown
    case OverlayButtonType::Jump:        return QColor("#00bcd4");  // cyan
    }
    return QColor("#2980b9");
}

QString OverlayButton::typeName(OverlayButtonType t)
{
    switch (t) {
    case OverlayButtonType::Click:       return "Fire (L-Click)";
    case OverlayButtonType::DoubleClick: return "Double Tap";
    case OverlayButtonType::RightClick:  return "Scope (R-Click)";
    case OverlayButtonType::MiddleClick: return "Mid Click";
    case OverlayButtonType::Joystick:    return "WASD Move";
    case OverlayButtonType::Aim:         return "Aim / Look";
    case OverlayButtonType::Swipe:       return "Swipe";
    case OverlayButtonType::FreeLook:    return "Free Look";
    case OverlayButtonType::Macro:       return "Macro";
    case OverlayButtonType::Spray:       return "Auto-Fire";
    case OverlayButtonType::Skill:       return "Skill";
    case OverlayButtonType::Map:         return "Map (M)";
    case OverlayButtonType::Bag:         return "Bag (Tab)";
    case OverlayButtonType::Scope:       return "Scope Zoom";
    case OverlayButtonType::Grenade:     return "Grenade";
    case OverlayButtonType::Vehicle:     return "Drive";
    case OverlayButtonType::Prone:       return "Prone/Crouch";
    case OverlayButtonType::Jump:        return "Jump";
    }
    return "Button";
}

QString OverlayButton::typeIcon(OverlayButtonType t)
{
    switch (t) {
    case OverlayButtonType::Click:       return "LMB";
    case OverlayButtonType::DoubleClick: return "x2";
    case OverlayButtonType::RightClick:  return "RMB";
    case OverlayButtonType::MiddleClick: return "MMB";
    case OverlayButtonType::Joystick:    return "JOY";
    case OverlayButtonType::Aim:         return "AIM";
    case OverlayButtonType::Swipe:       return "SWP";
    case OverlayButtonType::FreeLook:    return "CAM";
    case OverlayButtonType::Macro:       return "MCR";
    case OverlayButtonType::Spray:       return "AUTO";
    case OverlayButtonType::Skill:       return "SKL";
    case OverlayButtonType::Map:         return "MAP";
    case OverlayButtonType::Bag:         return "BAG";
    case OverlayButtonType::Scope:       return "SCO";
    case OverlayButtonType::Grenade:     return "GRN";
    case OverlayButtonType::Vehicle:     return "CAR";
    case OverlayButtonType::Prone:       return "PRN";
    case OverlayButtonType::Jump:        return "JMP";
    }
    return "BTN";
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
OverlayButton::OverlayButton(OverlayButtonType type,
                             const QString    &label,
                             const QString    &key,
                             QPointF           posRatio,
                             QWidget          *parent)
    : QWidget(parent)
    , m_type(type)
    , m_label(label)
    , m_key(key)
    , m_posRatio(posRatio)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setWindowFlags(Qt::FramelessWindowHint);
    int sz = 62;
    setFixedSize(sz, sz);
    setCursor(Qt::SizeAllCursor);
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------
void OverlayButton::setButtonType(OverlayButtonType t)  { m_type = t; update(); }
void OverlayButton::setLabel(const QString &l)           { m_label = l; update(); }
void OverlayButton::setKey(const QString &k)             { m_key = k; update(); }
void OverlayButton::setPosRatio(const QPointF &p)        { m_posRatio = p; }
void OverlayButton::setRadiusRatio(float r)              { m_radiusRatio = r; update(); }

void OverlayButton::setJoystickKeys(const QString &u, const QString &d, const QString &l, const QString &r)
{
    m_upKey = u; m_downKey = d; m_leftKey = l; m_rightKey = r;
}

void OverlayButton::setSpeedRatios(float x, float y) { m_speedRatioX = x; m_speedRatioY = y; }
void OverlayButton::setSwitchMap(bool sm)             { m_switchMap = sm; }
void OverlayButton::setHudOpacity(float op)           { m_hudOpacity = op; update(); }

void OverlayButton::setMacroSteps(const QList<MacroStep> &steps) { m_macroSteps = steps; }
void OverlayButton::addMacroStep(const MacroStep &step)           { m_macroSteps.append(step); }
void OverlayButton::clearMacroSteps()                             { m_macroSteps.clear(); }

void OverlayButton::setSprayIntervalMs(int ms) { m_sprayIntervalMs = ms; }
void OverlayButton::setSwipeEndRatio(const QPointF &p) { m_swipeEndRatio = p; }

void OverlayButton::setEditMode(bool edit)
{
    m_editMode = edit;
    setAttribute(Qt::WA_TransparentForMouseEvents, !edit);
    setCursor(edit ? Qt::SizeAllCursor : Qt::ArrowCursor);
    update();
}

void OverlayButton::setSelected(bool sel)
{
    m_selected = sel;
    update();
}

// ---------------------------------------------------------------------------
// Key helpers
// ---------------------------------------------------------------------------
QString OverlayButton::displayKey() const
{
    if (m_key.startsWith("Key_")) return m_key.mid(4);
    return m_key;
}

QString OverlayButton::mapArabicOrNumberToLatinKey(const QString &raw)
{
    if (raw.isEmpty()) return raw;
    static QHash<QString, QString> arabicMap {
        {QString::fromUtf8("\xd9\x82"), "Key_Q"},
        {QString::fromUtf8("\xd9\x88"), "Key_W"},
        {QString::fromUtf8("\xd9\x87"), "Key_E"},
        {QString::fromUtf8("\xd8\xb1"), "Key_R"},
        {QString::fromUtf8("\xd8\xaa"), "Key_T"},
    };
    QString trimmed = raw.trimmed();
    if (arabicMap.contains(trimmed)) return arabicMap[trimmed];
    return raw;
}

QString OverlayButton::normalizeKeyName(const QString &raw, const QString &defaultKey)
{
    if (raw.isEmpty()) return defaultKey;
    QString mapped = mapArabicOrNumberToLatinKey(raw);
    if (mapped.startsWith("Key_")) return mapped;
    if (mapped.length() == 1 && mapped[0].isLetter()) {
        return "Key_" + mapped.toUpper();
    }
    return mapped.isEmpty() ? defaultKey : mapped;
}

// ---------------------------------------------------------------------------
// Repositioning
// ---------------------------------------------------------------------------
int OverlayButton::baseRadius() const
{
    QRect area = currentTargetArea();
    int shortSide = qMin(area.width(), area.height());
    return static_cast<int>(shortSide * m_radiusRatio);
}

QRect OverlayButton::currentTargetArea() const
{
    if (!m_lastVideoArea.isEmpty()) return m_lastVideoArea;
    if (parentWidget()) return parentWidget()->rect();
    return QRect(0, 0, 400, 700);
}

void OverlayButton::reposition(const QRect &videoArea)
{
    m_lastVideoArea = videoArea;
    repositionFromRatio();
}

void OverlayButton::repositionFromRatio()
{
    QRect area = currentTargetArea();
    int cx = area.x() + static_cast<int>(m_posRatio.x() * area.width());
    int cy = area.y() + static_cast<int>(m_posRatio.y() * area.height());
    int r  = baseRadius();
    move(cx - r, cy - r);
    setFixedSize(r * 2, r * 2);
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------
void OverlayButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor baseColor = typeColor(m_type);
    float opacity = m_editMode ? 0.92f : m_hudOpacity;

    if (m_type == OverlayButtonType::Joystick || m_type == OverlayButtonType::Vehicle) {
        // Draw concentric circles for joystick
        QRectF outer = rect().adjusted(2, 2, -2, -2);
        QRectF inner = rect().adjusted(width()/3, height()/3, -width()/3, -height()/3);

        p.setPen(QPen(baseColor.lighter(130), 2.5));
        p.setBrush(QColor(0, 0, 0, static_cast<int>(120 * opacity)));
        p.drawEllipse(outer);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(baseColor.red(), baseColor.green(), baseColor.blue(),
                          static_cast<int>(180 * opacity)));
        p.drawEllipse(inner);

        // Direction arrows
        p.setPen(QPen(Qt::white, 1.5));
        int cx = width()/2, cy = height()/2, r = width()/2 - 8;
        QPointF dirs[] = {{(double)cx, (double)(cy-r+8)}, {(double)cx, (double)(cy+r-8)},
                           {(double)(cx-r+8), (double)cy}, {(double)(cx+r-8), (double)cy}};
        for (auto &d : dirs) {
            p.drawEllipse(d, 2.5, 2.5);
        }

    } else if (m_type == OverlayButtonType::Aim || m_type == OverlayButtonType::FreeLook) {
        // Crosshair circle
        QRectF rc = rect().adjusted(2, 2, -2, -2);
        p.setPen(QPen(baseColor.lighter(120), 2.5));
        p.setBrush(QColor(0, 0, 0, static_cast<int>(60 * opacity)));
        p.drawEllipse(rc);

        p.setPen(QPen(Qt::white, 1.5));
        int cx = width()/2, cy = height()/2, r = width()/4;
        p.drawLine(cx - r, cy, cx + r, cy);
        p.drawLine(cx, cy - r, cx, cy + r);

    } else {
        // Standard button
        QRectF rc = rect().adjusted(2, 2, -2, -2);

        QRadialGradient grad(rc.center(), rc.width() * 0.6);
        grad.setColorAt(0, baseColor.lighter(140));
        grad.setColorAt(1, baseColor.darker(130));

        if (m_selected && m_editMode) {
            p.setPen(QPen(Qt::white, 3));
        } else {
            p.setPen(QPen(baseColor.darker(160), 2));
        }
        p.setBrush(QColor(baseColor.red(), baseColor.green(), baseColor.blue(),
                          static_cast<int>(200 * opacity)));
        p.drawEllipse(rc);

        // Icon badge (top-right)
        if (m_editMode) {
            QRectF badge(rc.right() - 18, rc.top(), 18, 12);
            p.setBrush(QColor(0, 0, 0, 160));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(badge, 3, 3);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 5, QFont::Bold));
            p.drawText(badge, Qt::AlignCenter, typeIcon(m_type));
        }
    }

    // Label text
    p.setPen(Qt::white);
    QFont f("Segoe UI", 7, QFont::Bold);
    p.setFont(f);

    QString displayText = m_label.isEmpty() ? displayKey() : m_label;
    if (displayText.length() > 8) displayText = displayText.left(8) + "..";

    QRectF textRect = rect().adjusted(4, 4, -4, -4);
    p.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, displayText);

    // Selection glow
    if (m_selected && m_editMode) {
        p.setPen(QPen(QColor(255, 255, 100, 200), 3, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(rect().adjusted(1, 1, -1, -1));
    }
}

// ---------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------
void OverlayButton::mousePressEvent(QMouseEvent *e)
{
    if (!m_editMode) { QWidget::mousePressEvent(e); return; }
    if (e->button() == Qt::LeftButton) {
        m_dragging   = true;
        m_dragOffset = e->pos();
        raise();
        emit editRequested(this);
    }
}

void OverlayButton::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_editMode || !m_dragging) return;
    QPoint newPos = mapToParent(e->pos()) - m_dragOffset;

    // Clamp to video area
    QRect area = currentTargetArea();
    int r = baseRadius();
    newPos.setX(qBound(area.left(), newPos.x(), area.right() - r * 2));
    newPos.setY(qBound(area.top(), newPos.y(), area.bottom() - r * 2));
    move(newPos);

    // Update ratio
    int cx = newPos.x() + r - area.left();
    int cy = newPos.y() + r - area.top();
    if (area.width() > 0 && area.height() > 0) {
        m_posRatio = QPointF(
            static_cast<double>(cx) / area.width(),
            static_cast<double>(cy) / area.height()
        );
    }
    emit posRatioChanged(this);
}

void OverlayButton::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_editMode) { QWidget::mouseReleaseEvent(e); return; }
    if (e->button() == Qt::LeftButton) m_dragging = false;
}

void OverlayButton::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (!m_editMode) { QWidget::mouseDoubleClickEvent(e); return; }
    if (e->button() == Qt::LeftButton) emit editRequested(this);
}

void OverlayButton::contextMenuEvent(QContextMenuEvent *e)
{
    if (!m_editMode) return;
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: #1e2a3a; color: white; border: 1px solid #3a4a5a; border-radius: 6px; }"
        "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
        "QMenu::item:selected { background: #2563eb; }"
    );
    auto *editAction      = menu.addAction("Edit Properties");
    auto *dupAction       = menu.addAction("Duplicate");
    menu.addSeparator();
    auto *deleteAction    = menu.addAction("Delete");

    QAction *chosen = menu.exec(e->globalPos());
    if (chosen == editAction)   emit editRequested(this);
    if (chosen == dupAction)    emit duplicateRequested(this);
    if (chosen == deleteAction) emit deleteRequested(this);
}

// ---------------------------------------------------------------------------
// JSON Serialization
// ---------------------------------------------------------------------------
QJsonObject OverlayButton::toJson() const
{
    QJsonObject o;
    o["type"]           = static_cast<int>(m_type);
    o["label"]          = m_label;
    o["key"]            = m_key;
    o["posX"]           = m_posRatio.x();
    o["posY"]           = m_posRatio.y();
    o["radiusRatio"]    = static_cast<double>(m_radiusRatio);
    o["switchMap"]      = m_switchMap;
    o["hudOpacity"]     = static_cast<double>(m_hudOpacity);
    o["speedRatioX"]    = static_cast<double>(m_speedRatioX);
    o["speedRatioY"]    = static_cast<double>(m_speedRatioY);
    o["upKey"]          = m_upKey;
    o["downKey"]        = m_downKey;
    o["leftKey"]        = m_leftKey;
    o["rightKey"]       = m_rightKey;
    o["sprayInterval"]  = m_sprayIntervalMs;
    o["swipeEndX"]      = m_swipeEndRatio.x();
    o["swipeEndY"]      = m_swipeEndRatio.y();

    if (!m_macroSteps.isEmpty()) {
        QJsonArray arr;
        for (const auto &step : m_macroSteps) {
            QJsonObject s;
            s["key"]     = step.key;
            s["delay"]   = step.delayMs;
            s["press"]   = step.press;
            arr.append(s);
        }
        o["macroSteps"] = arr;
    }
    return o;
}

OverlayButton *OverlayButton::fromJson(const QJsonObject &obj, QWidget *parent)
{
    auto type = static_cast<OverlayButtonType>(obj["type"].toInt(0));
    QString label  = obj["label"].toString();
    QString key    = obj["key"].toString("Key_Space");
    double  px     = obj["posX"].toDouble(0.5);
    double  py     = obj["posY"].toDouble(0.5);

    auto *btn = new OverlayButton(type, label, key, QPointF(px, py), parent);
    btn->m_radiusRatio   = static_cast<float>(obj["radiusRatio"].toDouble(0.055));
    btn->m_switchMap     = obj["switchMap"].toBool(false);
    btn->m_hudOpacity    = static_cast<float>(obj["hudOpacity"].toDouble(0.85));
    btn->m_speedRatioX   = static_cast<float>(obj["speedRatioX"].toDouble(2.5));
    btn->m_speedRatioY   = static_cast<float>(obj["speedRatioY"].toDouble(2.5));
    btn->m_upKey         = obj["upKey"].toString("Key_W");
    btn->m_downKey       = obj["downKey"].toString("Key_S");
    btn->m_leftKey       = obj["leftKey"].toString("Key_A");
    btn->m_rightKey      = obj["rightKey"].toString("Key_D");
    btn->m_sprayIntervalMs = obj["sprayInterval"].toInt(80);
    btn->m_swipeEndRatio = QPointF(obj["swipeEndX"].toDouble(0.5), obj["swipeEndY"].toDouble(0.3));

    if (obj.contains("macroSteps")) {
        for (const auto &val : obj["macroSteps"].toArray()) {
            QJsonObject s = val.toObject();
            btn->m_macroSteps.append({s["key"].toString(), s["delay"].toInt(50), s["press"].toBool(true)});
        }
    }
    return btn;
}

QJsonObject OverlayButton::toKeyMapNode() const
{
    QJsonObject node;
    switch (m_type) {
    case OverlayButtonType::Click:
    case OverlayButtonType::DoubleClick:
    case OverlayButtonType::RightClick:
    case OverlayButtonType::MiddleClick:
    case OverlayButtonType::Map:
    case OverlayButtonType::Bag:
    case OverlayButtonType::Scope:
    case OverlayButtonType::Prone:
    case OverlayButtonType::Jump:
    case OverlayButtonType::Skill:
    case OverlayButtonType::Spray:
    {
        node["type"]    = (m_type == OverlayButtonType::DoubleClick) ? "KMT_CLICK_TWICE" : "KMT_CLICK";
        node["comment"] = m_label.isEmpty() ? typeName(m_type) : m_label;
        node["key"]     = m_key;
        QJsonObject pos; pos["x"] = m_posRatio.x(); pos["y"] = m_posRatio.y();
        node["pos"]       = pos;
        node["switchMap"] = m_switchMap;
        break;
    }
    case OverlayButtonType::Joystick:
    case OverlayButtonType::Vehicle:
    {
        node["type"]     = "KMT_STEER_WHEEL";
        node["comment"]  = m_label.isEmpty() ? typeName(m_type) : m_label;
        QJsonObject cp; cp["x"] = m_posRatio.x(); cp["y"] = m_posRatio.y();
        node["centerPos"] = cp;
        node["leftKey"]   = m_leftKey;
        node["rightKey"]  = m_rightKey;
        node["upKey"]     = m_upKey;
        node["downKey"]   = m_downKey;
        break;
    }
    case OverlayButtonType::Aim:
    case OverlayButtonType::FreeLook:
    {
        // Handled separately as mouseMoveMap
        node["type"]        = "mouseMoveMap";
        node["speedRatioX"] = static_cast<double>(m_speedRatioX);
        node["speedRatioY"] = static_cast<double>(m_speedRatioY);
        QJsonObject sp; sp["x"] = m_posRatio.x(); sp["y"] = m_posRatio.y();
        node["startPos"]    = sp;
        break;
    }
    case OverlayButtonType::Swipe:
    case OverlayButtonType::Grenade:
    {
        node["type"]    = "KMT_DRAG";
        node["comment"] = m_label.isEmpty() ? typeName(m_type) : m_label;
        node["key"]     = m_key;
        QJsonObject sp; sp["x"] = m_posRatio.x(); sp["y"] = m_posRatio.y();
        node["startPos"] = sp;
        QJsonObject ep; ep["x"] = m_swipeEndRatio.x(); ep["y"] = m_swipeEndRatio.y();
        node["endPos"]   = ep;
        break;
    }
    case OverlayButtonType::Macro:
    {
        node["type"]    = "KMT_CLICK";
        node["comment"] = m_label.isEmpty() ? "Macro" : m_label;
        node["key"]     = m_macroSteps.isEmpty() ? m_key : m_macroSteps.first().key;
        QJsonObject pos; pos["x"] = m_posRatio.x(); pos["y"] = m_posRatio.y();
        node["pos"]     = pos;
        break;
    }
    }
    return node;
}