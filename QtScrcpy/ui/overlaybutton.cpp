#include "overlaybutton.h"

#include <QContextMenuEvent>
#include <QFont>
#include <QJsonArray>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

// ---------------------------------------------------------------------------
// Construction
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
    setWindowFlags(Qt::FramelessWindowHint);

    setButtonType(type);
    setEditMode(false);
}

void OverlayButton::setButtonType(OverlayButtonType t)
{
    m_type = t;
    if (m_type == OverlayButtonType::Joystick) {
        m_radiusRatio = 0.11f;
        if (m_label.isEmpty()) m_label = "Move";
        if (m_key.isEmpty())   m_key   = "Key_W";
    } else if (m_type == OverlayButtonType::Aim) {
        m_radiusRatio = 0.08f;
        if (m_label.isEmpty()) m_label = "Aim";
        if (m_key.isEmpty())   m_key   = "Key_QuoteLeft";
    } else if (m_type == OverlayButtonType::DoubleClick) {
        m_radiusRatio = 0.055f;
        if (m_label.isEmpty()) m_label = "Double";
    } else {
        m_radiusRatio = 0.055f;
        if (m_label.isEmpty()) m_label = "Click";
    }
    repositionFromRatio();
    update();
}

// ---------------------------------------------------------------------------
// Accessors & Mutators
// ---------------------------------------------------------------------------
void OverlayButton::setLabel(const QString &l)
{
    m_label = l;
    update();
}

QString OverlayButton::mapArabicOrNumberToLatinKey(const QString &raw)
{
    QString s = raw.trimmed();
    if (s.startsWith("Key_")) s = s.mid(4);

    // Direct high-frequency game bindings
    if (s == "1579" || s == "ث") return "Key_E";
    if (s == "1602" || s == "ق") return "Key_R";
    if (s == "1589" || s == "ص") return "Key_W";
    if (s == "1588" || s == "ش") return "Key_A";
    if (s == "1587" || s == "س") return "Key_S";
    if (s == "1610" || s == "ي") return "Key_D";
    if (s == "1590" || s == "ض") return "Key_Q";
    if (s == "1601" || s == "ف") return "Key_T";
    if (s == "1594" || s == "غ") return "Key_Y";
    if (s == "1593" || s == "ع") return "Key_U";
    if (s == "1607" || s == "ه") return "Key_I";
    if (s == "1582" || s == "خ") return "Key_O";
    if (s == "1581" || s == "ح") return "Key_P";
    if (s == "1576" || s == "ب") return "Key_F";
    if (s == "1604" || s == "ل") return "Key_G";
    if (s == "1575" || s == "ا") return "Key_H";
    if (s == "1578" || s == "ت") return "Key_J";
    if (s == "1606" || s == "ن") return "Key_K";
    if (s == "1605" || s == "م") return "Key_L";
    if (s == "1574" || s == "ئ") return "Key_Z";
    if (s == "1569" || s == "ء") return "Key_X";
    if (s == "1572" || s == "ؤ") return "Key_C";
    if (s == "1585" || s == "ر") return "Key_V";
    if (s == "1609" || s == "ى") return "Key_N";
    if (s == "1577" || s == "ة") return "Key_M";
    if (s == "1584" || s == "ذ") return "Key_QuoteLeft";

    bool ok = false;
    int code = s.toInt(&ok);
    if (ok) {
        switch (code) {
        case 0x0636: return "Key_Q"; // ض
        case 0x0635: return "Key_W"; // ص
        case 0x062B: return "Key_E"; // ث (1579)
        case 0x0642: return "Key_R"; // ق (1602)
        case 0x0641: return "Key_T"; // ف (1601)
        case 0x063A: return "Key_Y"; // غ
        case 0x0639: return "Key_U"; // ع
        case 0x0647: return "Key_I"; // ه
        case 0x062E: return "Key_O"; // خ
        case 0x062D: return "Key_P"; // ح
        case 0x0634: return "Key_A"; // ش
        case 0x0633: return "Key_S"; // س
        case 0x064A: return "Key_D"; // ي
        case 0x0628: return "Key_F"; // ب
        case 0x0644: return "Key_G"; // ل
        case 0x0627: return "Key_H"; // ا
        case 0x062A: return "Key_J"; // ت
        case 0x0646: return "Key_K"; // ن
        case 0x0645: return "Key_L"; // م
        case 0x0626: return "Key_Z"; // ئ
        case 0x0621: return "Key_X"; // ء
        case 0x0624: return "Key_C"; // ؤ
        case 0x0631: return "Key_V"; // ر
        case 0x0649: return "Key_N"; // ى
        case 0x0629: return "Key_M"; // ة
        case 0x0630: return "Key_QuoteLeft"; // ذ
        default: break;
        }
    }
    return QString();
}

QString OverlayButton::normalizeKeyName(const QString &raw, const QString &defaultKey)
{
    QString k = raw.trimmed();
    if (k.isEmpty()) return defaultKey;
    if (k == "RightButton" || k == "LeftButton" || k == "MidButton") return k;
    if (k.compare("Right", Qt::CaseInsensitive) == 0 || k == "RMB") return "RightButton";
    if (k.compare("Left", Qt::CaseInsensitive) == 0 || k == "LMB") return "LeftButton";
    if (k.compare("Middle", Qt::CaseInsensitive) == 0 || k == "MMB") return "MidButton";

    QString mapped = mapArabicOrNumberToLatinKey(k);
    if (!mapped.isEmpty()) {
        return mapped;
    }

    if (!k.startsWith("Key_")) {
        if (k.length() == 1) {
            return "Key_" + k.toUpper();
        }
        return "Key_" + k;
    }
    return k;
}

void OverlayButton::setKey(const QString &k)
{
    m_key = normalizeKeyName(k, "Key_J");
    update();
}

void OverlayButton::setHudOpacity(float op)
{
    m_hudOpacity = qBound(0.15f, op, 1.0f);
    update();
}

void OverlayButton::setPosRatio(const QPointF &p)
{
    m_posRatio = QPointF(qBound(0.0, p.x(), 1.0), qBound(0.0, p.y(), 1.0));
    repositionFromRatio();
}

void OverlayButton::setRadiusRatio(float r)
{
    m_radiusRatio = qBound(0.02f, r, 0.35f);
    repositionFromRatio();
}

void OverlayButton::setJoystickKeys(const QString &u, const QString &d, const QString &l, const QString &r)
{
    m_upKey    = normalizeKeyName(u, "Key_W");
    m_downKey  = normalizeKeyName(d, "Key_S");
    m_leftKey  = normalizeKeyName(l, "Key_A");
    m_rightKey = normalizeKeyName(r, "Key_D");
    update();
}

void OverlayButton::setSpeedRatios(float x, float y)
{
    m_speedRatioX = x;
    m_speedRatioY = y;
    update();
}

void OverlayButton::setSwitchMap(bool sm)
{
    m_switchMap = sm;
    update();
}

void OverlayButton::setEditMode(bool edit)
{
    m_editMode = edit;
    setMouseTracking(edit);
    setAttribute(Qt::WA_TransparentForMouseEvents, !edit);
    setCursor(edit ? Qt::SizeAllCursor : Qt::ArrowCursor);
    update();
}

void OverlayButton::setSelected(bool sel)
{
    m_selected = sel;
    update();
}

QString OverlayButton::displayKey() const
{
    if (m_type == OverlayButtonType::Joystick) return "WASD";
    if (m_type == OverlayButtonType::Aim)      return "AIM";

    QString k = m_key;
    if (k == "LeftButton" || k == "Left") {
        return (m_label.compare("Fire", Qt::CaseInsensitive) == 0) ? "FIRE" : "LMB";
    }
    if (k == "RightButton" || k == "Right") {
        return (m_label.compare("Scope", Qt::CaseInsensitive) == 0) ? "SCOPE" : "RMB";
    }
    if (k == "MidButton" || k == "Middle") return "MMB";

    QString mapped = mapArabicOrNumberToLatinKey(k);
    if (!mapped.isEmpty()) {
        return mapped.mid(4); // e.g. "E" for 1579, "R" for 1602
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
    if (k == "CapsLock")  return "CAPS";
    return k.toUpper();
}

// ---------------------------------------------------------------------------
// Layout & Positioning
// ---------------------------------------------------------------------------
QRect OverlayButton::currentTargetArea() const
{
    if (m_lastVideoArea.isValid() && !m_lastVideoArea.isEmpty()) {
        return m_lastVideoArea;
    }
    if (parentWidget()) {
        return parentWidget()->rect();
    }
    return QRect(0, 0, 800, 600);
}

int OverlayButton::baseRadius() const
{
    QRect area = currentTargetArea();
    int minDim = qMin(area.width(), area.height());
    return qMax(18, static_cast<int>(minDim * m_radiusRatio));
}

void OverlayButton::reposition(const QRect &videoArea)
{
    m_lastVideoArea = videoArea;
    repositionFromRatio();
}

void OverlayButton::repositionFromRatio()
{
    QRect area = currentTargetArea();
    int r = baseRadius();
    int cx = area.x() + static_cast<int>(m_posRatio.x() * area.width());
    int cy = area.y() + static_cast<int>(m_posRatio.y() * area.height());
    int size = r * 2 + 8;
    setGeometry(cx - r - 4, cy - r - 4, size, size);
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------
void OverlayButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // LIVE OPACITY: Dynamically scale transparency with m_hudOpacity!
    // In edit mode we keep a gentle floor (0.22) so it's always interactable, while visually responding live!
    float effectiveOpacity = m_editMode ? qMax(0.22f, m_hudOpacity) : m_hudOpacity;
    p.setOpacity(static_cast<qreal>(effectiveOpacity));

    int r = baseRadius();
    int cx = width() / 2;
    int cy = height() / 2;

    // Palette per type
    QColor mainColor;
    QColor borderColor;
    QColor glowColor;

    switch (m_type) {
    case OverlayButtonType::Joystick:
        mainColor   = QColor(14, 165, 233, m_editMode ? 140 : 80);
        borderColor = QColor(56, 189, 248, m_editMode ? 230 : 160);
        glowColor   = QColor(14, 165, 233, 100);
        break;
    case OverlayButtonType::Aim:
        mainColor   = QColor(239, 68, 68, m_editMode ? 140 : 85);
        borderColor = QColor(248, 113, 113, m_editMode ? 240 : 170);
        glowColor   = QColor(239, 68, 68, 110);
        break;
    case OverlayButtonType::DoubleClick:
        mainColor   = QColor(168, 85, 247, m_editMode ? 145 : 85);
        borderColor = QColor(192, 132, 252, m_editMode ? 240 : 170);
        glowColor   = QColor(168, 85, 247, 100);
        break;
    case OverlayButtonType::Swipe:
        mainColor   = QColor(234, 179, 8, m_editMode ? 140 : 85);
        borderColor = QColor(250, 204, 21, m_editMode ? 235 : 170);
        glowColor   = QColor(234, 179, 8, 100);
        break;
    case OverlayButtonType::Click:
    default:
        if (m_key == "LeftButton" || m_label.compare("Fire", Qt::CaseInsensitive) == 0) {
            // Weapon Fire Theme
            mainColor   = QColor(225, 29, 72, m_editMode ? 160 : 100);
            borderColor = QColor(251, 113, 133, m_editMode ? 245 : 180);
            glowColor   = QColor(225, 29, 72, 120);
        } else if (m_key == "RightButton" || m_label.compare("Scope", Qt::CaseInsensitive) == 0) {
            // Scope / Aim Down Sights Theme
            mainColor   = QColor(13, 148, 136, m_editMode ? 160 : 100);
            borderColor = QColor(45, 212, 191, m_editMode ? 245 : 180);
            glowColor   = QColor(13, 148, 136, 120);
        } else {
            mainColor   = QColor(37, 99, 235, m_editMode ? 145 : 85);
            borderColor = QColor(96, 165, 250, m_editMode ? 240 : 170);
            glowColor   = QColor(37, 99, 235, 110);
        }
        break;
    }

    if (m_selected && m_editMode) {
        mainColor   = QColor(245, 158, 11, 165);
        borderColor = QColor(251, 191, 36, 255);
        glowColor   = QColor(245, 158, 11, 140);
    }

    // Outer glow for edit mode or selected
    if (m_editMode) {
        p.setPen(Qt::NoPen);
        p.setBrush(glowColor);
        p.drawEllipse(QPoint(cx, cy), r + 3, r + 3);
    }

    // Main button background
    p.setPen(QPen(borderColor, m_selected ? 2.5 : 1.8));
    p.setBrush(mainColor);
    p.drawEllipse(QPoint(cx, cy), r, r);

    // Type-specific decorations
    if (m_type == OverlayButtonType::Joystick) {
        // Inner thumb nub
        p.setPen(QPen(QColor(255, 255, 255, 120), 1.2));
        p.setBrush(QColor(255, 255, 255, 50));
        p.drawEllipse(QPoint(cx, cy), r / 3, r / 3);

        // Direction cross lines
        p.setPen(QPen(QColor(255, 255, 255, 70), 1, Qt::DashLine));
        p.drawLine(cx, cy - r + 4, cx, cy + r - 4);
        p.drawLine(cx - r + 4, cy, cx + r - 4, cy);

        // Direction markers W A S D
        QFont dFont = p.font();
        dFont.setBold(true);
        dFont.setPixelSize(qMax(8, r / 4));
        p.setFont(dFont);
        p.setPen(QColor(255, 255, 255, 200));

        p.drawText(QRect(cx - 10, cy - r + 3, 20, 14), Qt::AlignCenter, "W");
        p.drawText(QRect(cx - 10, cy + r - 17, 20, 14), Qt::AlignCenter, "S");
        p.drawText(QRect(cx - r + 3, cy - 7, 14, 14), Qt::AlignCenter, "A");
        p.drawText(QRect(cx + r - 17, cy - 7, 14, 14), Qt::AlignCenter, "D");

    } else if (m_type == OverlayButtonType::Aim) {
        // Tactical Crosshair reticle
        p.setPen(QPen(QColor(255, 255, 255, 220), 1.5));
        int crossLen = r / 2;
        p.drawLine(cx, cy - crossLen, cx, cy - 4);
        p.drawLine(cx, cy + 4, cx, cy + crossLen);
        p.drawLine(cx - crossLen, cy, cx - 4, cy);
        p.drawLine(cx + 4, cy, cx + crossLen, cy);
        // Center dot
        p.setBrush(Qt::white);
        p.drawEllipse(QPoint(cx, cy), 2, 2);

    } else if (m_type == OverlayButtonType::DoubleClick) {
        // Concentric inner ring
        p.setPen(QPen(QColor(255, 255, 255, 140), 1.2, Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPoint(cx, cy), r - 5, r - 5);

        // 2X badge in top right
        QFont bFont = p.font();
        bFont.setBold(true);
        bFont.setPixelSize(qMax(8, r / 3));
        p.setFont(bFont);
        p.setPen(QColor(255, 220, 0, 240));
        p.drawText(QRect(cx + r / 4, cy - r + 2, r / 2 + 6, r / 3 + 2), Qt::AlignCenter, "2x");
    }

    // Text: Key name & Label
    if (m_type != OverlayButtonType::Joystick) {
        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(qMax(10, static_cast<int>(r * 0.48)));
        p.setFont(f);

        // Drop shadow for text
        p.setPen(QColor(0, 0, 0, 180));
        QRect textRect(cx - r + 1, cy - r / 2 + 1, (r * 2), r);
        p.drawText(textRect, Qt::AlignCenter, displayKey());

        // Bright foreground text
        p.setPen(Qt::white);
        textRect = QRect(cx - r, cy - r / 2, (r * 2), r);
        p.drawText(textRect, Qt::AlignCenter, displayKey());
    }

    // In Edit mode: draw friendly label underneath
    if (m_editMode && !m_label.isEmpty()) {
        QFont lFont = p.font();
        lFont.setBold(false);
        lFont.setPixelSize(qMax(8, r / 4));
        p.setFont(lFont);

        p.setPen(QColor(0, 0, 0, 200));
        p.drawText(QRect(cx - r + 1, cy + r / 4 + 1, r * 2, r / 2), Qt::AlignCenter, m_label);
        p.setPen(QColor(240, 240, 240, 220));
        p.drawText(QRect(cx - r, cy + r / 4, r * 2, r / 2), Qt::AlignCenter, m_label);
    }

    // Selection dashed boundary
    if (m_editMode && m_selected) {
        p.setPen(QPen(QColor(255, 255, 255, 230), 1.5, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRect(2, 2, width() - 4, height() - 4);

        // Corner handles
        p.setBrush(QColor(251, 191, 36));
        p.setPen(Qt::NoPen);
        p.drawRect(0, 0, 4, 4);
        p.drawRect(width() - 4, 0, 4, 4);
        p.drawRect(0, height() - 4, 4, 4);
        p.drawRect(width() - 4, height() - 4, 4, 4);
    }
}

// ---------------------------------------------------------------------------
// Mouse Events
// ---------------------------------------------------------------------------
void OverlayButton::mousePressEvent(QMouseEvent *e)
{
    if (m_editMode) {
        if (e->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragOffset = e->pos();
            setSelected(true);
            emit editRequested(this);
            raise();
            e->accept();
            return;
        }
    }
    QWidget::mousePressEvent(e);
}

void OverlayButton::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && m_editMode && parentWidget()) {
        QRect area = currentTargetArea();
        QPoint globalPos = mapToGlobal(e->pos());
        QPoint localInParent = parentWidget()->mapFromGlobal(globalPos) - m_dragOffset;

        int r = baseRadius();
        int newCenterX = localInParent.x() + r + 4;
        int newCenterY = localInParent.y() + r + 4;

        // Clamp inside video area
        newCenterX = qBound(area.left(), newCenterX, area.right());
        newCenterY = qBound(area.top(),  newCenterY, area.bottom());

        double ratioX = static_cast<double>(newCenterX - area.left()) / qMax(1, area.width());
        double ratioY = static_cast<double>(newCenterY - area.top())  / qMax(1, area.height());

        m_posRatio = QPointF(qBound(0.0, ratioX, 1.0), qBound(0.0, ratioY, 1.0));
        repositionFromRatio();
        emit posRatioChanged(this);
        e->accept();
        return;
    }
    QWidget::mouseMoveEvent(e);
}

void OverlayButton::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(e);
}

void OverlayButton::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (m_editMode) {
        setSelected(true);
        emit editRequested(this);
        e->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(e);
}

void OverlayButton::contextMenuEvent(QContextMenuEvent *e)
{
    if (!m_editMode) {
        QWidget::contextMenuEvent(e);
        return;
    }

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #1e293b; color: #f8fafc; border: 1px solid #334155; border-radius: 6px; padding: 4px; }"
        "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: #3b82f6; }"
    );

    QAction *actEdit = menu.addAction(tr("✏️ Edit Properties"));
    QAction *actDup  = menu.addAction(tr("📋 Duplicate"));
    menu.addSeparator();
    QAction *actDel  = menu.addAction(tr("🗑️ Delete"));

    QAction *chosen = menu.exec(e->globalPos());
    if (chosen == actEdit) {
        setSelected(true);
        emit editRequested(this);
    } else if (chosen == actDup) {
        emit duplicateRequested(this);
    } else if (chosen == actDel) {
        emit deleteRequested(this);
    }
    e->accept();
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------
QJsonObject OverlayButton::toJson() const
{
    QJsonObject o;
    o["label"]       = m_label;
    o["key"]         = m_key;
    o["posX"]        = m_posRatio.x();
    o["posY"]        = m_posRatio.y();
    o["radius"]      = static_cast<double>(m_radiusRatio);
    o["upKey"]       = m_upKey;
    o["downKey"]     = m_downKey;
    o["leftKey"]     = m_leftKey;
    o["rightKey"]    = m_rightKey;
    o["speedRatioX"] = static_cast<double>(m_speedRatioX);
    o["speedRatioY"] = static_cast<double>(m_speedRatioY);
    o["switchMap"]   = m_switchMap;
    o["opacity"]     = static_cast<double>(m_hudOpacity);

    QString t = "click";
    if (m_type == OverlayButtonType::DoubleClick) t = "double_click";
    else if (m_type == OverlayButtonType::Joystick)    t = "joystick";
    else if (m_type == OverlayButtonType::Aim)         t = "aim";
    else if (m_type == OverlayButtonType::Swipe)       t = "swipe";

    o["type"] = t;
    return o;
}

OverlayButton *OverlayButton::fromJson(const QJsonObject &o, QWidget *parent)
{
    QString typeStr = o["type"].toString("click");
    OverlayButtonType t = OverlayButtonType::Click;
    if (typeStr == "double_click")   t = OverlayButtonType::DoubleClick;
    else if (typeStr == "joystick")  t = OverlayButtonType::Joystick;
    else if (typeStr == "aim")       t = OverlayButtonType::Aim;
    else if (typeStr == "swipe")     t = OverlayButtonType::Swipe;

    auto *btn = new OverlayButton(
        t,
        o["label"].toString("Key"),
        o["key"].toString("Key_J"),
        QPointF(o["posX"].toDouble(0.5), o["posY"].toDouble(0.5)),
        parent
    );

    if (o.contains("radius"))      btn->setRadiusRatio(static_cast<float>(o["radius"].toDouble()));
    if (o.contains("upKey"))       btn->m_upKey    = o["upKey"].toString("Key_W");
    if (o.contains("downKey"))     btn->m_downKey  = o["downKey"].toString("Key_S");
    if (o.contains("leftKey"))     btn->m_leftKey  = o["leftKey"].toString("Key_A");
    if (o.contains("rightKey"))    btn->m_rightKey = o["rightKey"].toString("Key_D");
    if (o.contains("speedRatioX")) btn->m_speedRatioX = static_cast<float>(o["speedRatioX"].toDouble(2.5));
    if (o.contains("speedRatioY")) btn->m_speedRatioY = static_cast<float>(o["speedRatioY"].toDouble(2.5));
    if (o.contains("switchMap"))   btn->m_switchMap = o["switchMap"].toBool(false);
    if (o.contains("opacity"))     btn->setHudOpacity(static_cast<float>(o["opacity"].toDouble(0.85)));

    return btn;
}

QJsonObject OverlayButton::toKeyMapNode() const
{
    QJsonObject node;
    node["comment"] = m_label;

    if (m_type == OverlayButtonType::Click) {
        node["type"] = "KMT_CLICK";
        node["key"]  = normalizeKeyName(m_key, "Key_J");
        QJsonObject pos;
        pos["x"] = m_posRatio.x();
        pos["y"] = m_posRatio.y();
        node["pos"] = pos;
        node["switchMap"] = m_switchMap;

    } else if (m_type == OverlayButtonType::DoubleClick) {
        node["type"] = "KMT_CLICK_TWICE";
        node["key"]  = normalizeKeyName(m_key, "Key_Q");
        QJsonObject pos;
        pos["x"] = m_posRatio.x();
        pos["y"] = m_posRatio.y();
        node["pos"] = pos;

    } else if (m_type == OverlayButtonType::Joystick) {
        node["type"] = "KMT_STEER_WHEEL";
        QJsonObject center;
        center["x"] = m_posRatio.x();
        center["y"] = m_posRatio.y();
        node["centerPos"]   = center;

        // Dynamic offsets calculated from joystick radius for genuine movement and sprinting
        double rad = qMax(0.06, static_cast<double>(m_radiusRatio));
        node["leftOffset"]  = qBound(0.08, rad * 1.6, 0.35);
        node["rightOffset"] = qBound(0.08, rad * 1.6, 0.35);
        node["upOffset"]    = qBound(0.12, rad * 2.2, 0.40); // Higher forward throw triggers run/sprint in PUBG/FPS games
        node["downOffset"]  = qBound(0.08, rad * 1.6, 0.35);

        node["leftKey"]     = normalizeKeyName(m_leftKey,  "Key_A");
        node["rightKey"]    = normalizeKeyName(m_rightKey, "Key_D");
        node["upKey"]       = normalizeKeyName(m_upKey,    "Key_W");
        node["downKey"]     = normalizeKeyName(m_downKey,  "Key_S");

    } else if (m_type == OverlayButtonType::Swipe) {
        node["type"] = "KMT_DRAG";
        node["key"]  = m_key.isEmpty() ? "Key_Up" : m_key;
        QJsonObject startPos;
        startPos["x"] = m_posRatio.x();
        startPos["y"] = m_posRatio.y();
        node["startPos"] = startPos;
        QJsonObject endPos;
        endPos["x"] = m_posRatio.x();
        endPos["y"] = qMax(0.0, m_posRatio.y() - 0.25);
        node["endPos"] = endPos;
    }

    return node;
}
