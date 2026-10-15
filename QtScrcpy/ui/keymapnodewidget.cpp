#include "keymapnodewidget.h"
#include "keycapturedlg.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QDialog>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>

static const int CLOSE_BTN_SIZE   = 18;
static const int CLOSE_BTN_MARGIN = 3;

static QJsonObject normalizedCenter(const QWidget *widget)
{
    QJsonObject position;
    const QWidget *parent = widget->parentWidget();
    if (parent && !parent->size().isEmpty()) {
        position[QStringLiteral("x")] =
            static_cast<double>(widget->geometry().center().x()) / parent->width();
        position[QStringLiteral("y")] =
            static_cast<double>(widget->geometry().center().y()) / parent->height();
    } else {
        position[QStringLiteral("x")] = 0.5;
        position[QStringLiteral("y")] = 0.5;
    }
    return position;
}

static void moveToNormalizedCenter(QWidget *widget, const QJsonObject &position)
{
    QWidget *parent = widget->parentWidget();
    if (!parent || parent->size().isEmpty()) {
        return;
    }
    const double xRatio = qBound(0.0, position.value(QStringLiteral("x")).toDouble(0.5), 1.0);
    const double yRatio = qBound(0.0, position.value(QStringLiteral("y")).toDouble(0.5), 1.0);
    const int x = qRound(xRatio * parent->width() - widget->width() / 2.0);
    const int y = qRound(yRatio * parent->height() - widget->height() / 2.0);
    widget->move(qBound(0, x, qMax(0, parent->width() - widget->width())),
                 qBound(0, y, qMax(0, parent->height() - widget->height())));
}

// ===========================================================================
// KeymapNodeWidget (base)
// ===========================================================================

KeymapNodeWidget::KeymapNodeWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(64, 64);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
}

KeymapNodeWidget::~KeymapNodeWidget()
{
}

void KeymapNodeWidget::setKeyName(const QString &name)
{
    m_keyName = name;
    update();
}

QRect KeymapNodeWidget::closeButtonRect() const
{
    return QRect(width() - CLOSE_BTN_SIZE - CLOSE_BTN_MARGIN,
                 CLOSE_BTN_MARGIN,
                 CLOSE_BTN_SIZE,
                 CLOSE_BTN_SIZE);
}

void KeymapNodeWidget::paintCloseButton(QPainter &p) const
{
    QRect r = closeButtonRect();
    p.setPen(Qt::NoPen);
    p.setBrush(m_closeHovered ? QColor(220, 50, 50) : QColor(180, 30, 30, 200));
    p.drawEllipse(r);
    p.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap));
    int margin = 4;
    p.drawLine(r.left() + margin, r.top() + margin, r.right() - margin, r.bottom() - margin);
    p.drawLine(r.right() - margin, r.top() + margin, r.left() + margin, r.bottom() - margin);
}

void KeymapNodeWidget::captureKey()
{
    KeyCaptureDlg dlg(this);
    if (dlg.exec() == QDialog::Accepted && !dlg.capturedKey().isEmpty()) {
        setKeyName(dlg.capturedKey());
    }
}

void KeymapNodeWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (closeButtonRect().contains(event->pos())) {
            emit removeRequested();
            event->accept();
            return;
        }
        m_dragStartPosition = event->pos();
        m_isDragging = true;
        raise();
    }
    event->accept();
}

void KeymapNodeWidget::mouseMoveEvent(QMouseEvent *event)
{
    bool wasHovered = m_closeHovered;
    m_closeHovered = closeButtonRect().contains(event->pos());
    if (m_closeHovered != wasHovered) update();

    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        QPoint newPos = pos() + (event->pos() - m_dragStartPosition);
        if (parentWidget()) {
            int maxX = qMax(0, parentWidget()->width() - width());
            int maxY = qMax(0, parentWidget()->height() - height());
            newPos.setX(qBound(0, newPos.x(), maxX));
            newPos.setY(qBound(0, newPos.y(), maxY));
        }
        move(newPos);
    }
    event->accept();
}

void KeymapNodeWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        emit interactionFinished();
    }
    event->accept();
}

void KeymapNodeWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !closeButtonRect().contains(event->pos())) {
        captureKey();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void KeymapNodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(0, 200, 255), 2));
    p.setBrush(QColor(0, 120, 200, 160));
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));

    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPointSize(8);
    f.setBold(true);
    p.setFont(f);
    QRect textRect = rect().adjusted(2, 8, -2, -2);
    p.drawText(textRect, Qt::AlignCenter,
               m_keyName.isEmpty() ? QStringLiteral("?") : m_keyName);

    paintCloseButton(p);
}

// ===========================================================================
// ClickNodeWidget
// ===========================================================================

ClickNodeWidget::ClickNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    m_keyName = QStringLiteral("Key_F");
}

void ClickNodeWidget::setActionType(const QString &type)
{
    m_originalJson[QStringLiteral("type")] = type;
    update();
}

QJsonObject ClickNodeWidget::toJson() const
{
    QJsonObject obj = m_originalJson;
    if (!obj.contains(QStringLiteral("type"))
        || obj.value(QStringLiteral("type")).toString() == QStringLiteral("click")) {
        obj[QStringLiteral("type")] = QStringLiteral("KMT_CLICK");
    }
    obj[QStringLiteral("pos")] = normalizedCenter(this);
    obj[QStringLiteral("key")] = m_keyName;
    if (!obj.value(QStringLiteral("switchMap")).isBool()) {
        obj[QStringLiteral("switchMap")] = false;
    }
    return obj;
}

void ClickNodeWidget::fromJson(const QJsonObject &json)
{
    m_originalJson = json;
    if (json.contains(QStringLiteral("key"))) {
        m_keyName = json[QStringLiteral("key")].toString();
    }
    if (json.value(QStringLiteral("pos")).isObject()) {
        moveToNormalizedCenter(this, json.value(QStringLiteral("pos")).toObject());
    }
}

void ClickNodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor ringColor(0, 200, 255);
    QColor fillColor(0, 80, 180, 160);
    QString display = m_keyName;

    if (m_keyName == QStringLiteral("LeftButton")) {
        ringColor = QColor(255, 160, 0);
        fillColor = QColor(160, 80, 0, 160);
        display   = QStringLiteral("FIRE\n(LClick)");
    } else if (m_keyName == QStringLiteral("RightButton")) {
        ringColor = QColor(180, 0, 255);
        fillColor = QColor(80, 0, 160, 160);
        display   = QStringLiteral("SCOPE\n(RClick)");
    } else {
        // Show clean key name (strip "Key_" prefix)
        display = m_keyName;
        if (display.startsWith(QStringLiteral("Key_")))
            display = display.mid(4);
    }
    if (m_originalJson.value(QStringLiteral("type")).toString()
        == QStringLiteral("KMT_CLICK_TWICE")) {
        display = QStringLiteral("DOUBLE\n") + display;
        ringColor = QColor(255, 200, 0);
        fillColor = QColor(150, 100, 0, 160);
    }

    p.setPen(QPen(ringColor, 2));
    p.setBrush(fillColor);
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));

    p.setPen(Qt::NoPen);
    p.setBrush(ringColor);
    p.drawEllipse(rect().center(), 4, 4);

    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPointSize(7);
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect().adjusted(2, 6, -2, -4), Qt::AlignCenter, display);

    paintCloseButton(p);
}

DragNodeWidget::DragNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    setFixedSize(70, 70);
    m_keyName = QStringLiteral("Key_F");
}

QJsonObject DragNodeWidget::toJson() const
{
    QJsonObject obj = m_originalJson;
    obj[QStringLiteral("type")] = QStringLiteral("KMT_DRAG");
    obj[QStringLiteral("key")] = m_keyName;
    obj[QStringLiteral("startPos")] = normalizedCenter(this);
    QJsonObject endPos;
    endPos[QStringLiteral("x")] = m_endPosition.x();
    endPos[QStringLiteral("y")] = m_endPosition.y();
    obj[QStringLiteral("endPos")] = endPos;
    if (!obj.value(QStringLiteral("dragSpeed")).isDouble()
        || obj.value(QStringLiteral("dragSpeed")).toDouble() < 0.0
        || obj.value(QStringLiteral("dragSpeed")).toDouble() > 1.0) {
        obj[QStringLiteral("dragSpeed")] = 1.0;
    }
    if (!obj.value(QStringLiteral("startDelay")).isDouble()
        || obj.value(QStringLiteral("startDelay")).toDouble() < 0.0
        || obj.value(QStringLiteral("startDelay")).toDouble() > 4294967295.0) {
        obj[QStringLiteral("startDelay")] = 0;
    }
    return obj;
}

void DragNodeWidget::fromJson(const QJsonObject &json)
{
    m_originalJson = json;
    m_keyName = json.value(QStringLiteral("key")).toString(QStringLiteral("Key_F"));
    if (json.value(QStringLiteral("startPos")).isObject()) {
        moveToNormalizedCenter(this, json.value(QStringLiteral("startPos")).toObject());
    }
    if (json.value(QStringLiteral("endPos")).isObject()) {
        const QJsonObject endPos = json.value(QStringLiteral("endPos")).toObject();
        m_endPosition.setX(qBound(0.0, endPos.value(QStringLiteral("x")).toDouble(0.75), 1.0));
        m_endPosition.setY(qBound(0.0, endPos.value(QStringLiteral("y")).toDouble(0.5), 1.0));
    }
    update();
}

void DragNodeWidget::setEndPoint(const QPoint &point)
{
    if (!parentWidget() || parentWidget()->size().isEmpty()) {
        return;
    }
    m_endPosition.setX(qBound(0.0, static_cast<double>(point.x()) / parentWidget()->width(), 1.0));
    m_endPosition.setY(qBound(0.0, static_cast<double>(point.y()) / parentWidget()->height(), 1.0));
    update();
    parentWidget()->update();
}

void DragNodeWidget::paintGuide(QPainter &painter) const
{
    if (!parentWidget()) {
        return;
    }
    const QPoint start = geometry().center();
    const QPoint end(qRound(m_endPosition.x() * parentWidget()->width()),
                     qRound(m_endPosition.y() * parentWidget()->height()));
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 170, 70, 220), 3, Qt::DashLine, Qt::RoundCap));
    painter.drawLine(start, end);
    painter.setPen(QPen(QColor(255, 220, 150), 2));
    painter.setBrush(QColor(255, 140, 40, 180));
    painter.drawEllipse(end, 8, 8);
}

void DragNodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(255, 170, 70), 2));
    p.setBrush(QColor(150, 75, 20, 210));
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));
    p.setPen(Qt::white);
    QFont font = p.font();
    font.setPointSize(8);
    font.setBold(true);
    p.setFont(font);
    QString label = m_keyName.startsWith(QStringLiteral("Key_")) ? m_keyName.mid(4) : m_keyName;
    p.drawText(rect().adjusted(2, 8, -2, -2), Qt::AlignCenter, tr("SLIDE\n") + label);
    paintCloseButton(p);
}

void DragNodeWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint oldPos = pos();
    KeymapNodeWidget::mouseMoveEvent(event);
    const QPoint delta = pos() - oldPos;
    if (!delta.isNull() && parentWidget() && !parentWidget()->size().isEmpty()) {
        m_endPosition.setX(qBound(0.0, m_endPosition.x()
            + static_cast<double>(delta.x()) / parentWidget()->width(), 1.0));
        m_endPosition.setY(qBound(0.0, m_endPosition.y()
            + static_cast<double>(delta.y()) / parentWidget()->height(), 1.0));
        parentWidget()->update();
    }
}

MultiClickNodeWidget::MultiClickNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    setFixedSize(70, 70);
    m_keyName = QStringLiteral("Key_Space");
}

QJsonObject MultiClickNodeWidget::toJson() const
{
    QJsonObject obj = m_originalJson;
    obj[QStringLiteral("type")] = QStringLiteral("KMT_CLICK_MULTI");
    obj[QStringLiteral("key")] = m_keyName;
    QJsonArray clickNodes;
    for (int i = 0; i < m_clickPositions.size(); ++i) {
        QJsonObject pos;
        pos[QStringLiteral("x")] = m_clickPositions.at(i).x();
        pos[QStringLiteral("y")] = m_clickPositions.at(i).y();
        QJsonObject click;
        click[QStringLiteral("delay")] = m_clickDelays.value(i, i == 0 ? 0 : 120);
        click[QStringLiteral("pos")] = pos;
        clickNodes.append(click);
    }
    obj[QStringLiteral("clickNodes")] = clickNodes;
    return obj;
}

void MultiClickNodeWidget::fromJson(const QJsonObject &json)
{
    m_originalJson = json;
    m_keyName = json.value(QStringLiteral("key")).toString(QStringLiteral("Key_Space"));
    const QJsonArray clickNodes = json.value(QStringLiteral("clickNodes")).toArray();
    for (const QJsonValue &value : clickNodes) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject click = value.toObject();
        const QJsonValue posValue = click.value(QStringLiteral("pos"));
        if (!posValue.isObject() || !click.value(QStringLiteral("delay")).isDouble()) {
            continue;
        }
        const QJsonObject pos = posValue.toObject();
        if (!pos.value(QStringLiteral("x")).isDouble() || !pos.value(QStringLiteral("y")).isDouble()) {
            continue;
        }
        m_clickPositions.append(QPointF(
            pos.value(QStringLiteral("x")).toDouble(),
            pos.value(QStringLiteral("y")).toDouble()));
        m_clickDelays.append(static_cast<int>(click.value(QStringLiteral("delay")).toDouble()));
    }
    if (!m_clickPositions.isEmpty()) {
        QJsonObject firstPos;
        firstPos[QStringLiteral("x")] = m_clickPositions.first().x();
        firstPos[QStringLiteral("y")] = m_clickPositions.first().y();
        moveToNormalizedCenter(this, firstPos);
    }
    update();
}

bool MultiClickNodeWidget::addClickPoint(const QPoint &point)
{
    if (!parentWidget() || parentWidget()->size().isEmpty() || m_clickPositions.size() >= 50) {
        return false;
    }
    const QPointF normalized(
        qBound(0.0, static_cast<double>(point.x()) / parentWidget()->width(), 1.0),
        qBound(0.0, static_cast<double>(point.y()) / parentWidget()->height(), 1.0));
    m_clickPositions.append(normalized);
    m_clickDelays.append(m_clickDelays.isEmpty() ? 0 : 120);
    if (m_clickPositions.size() == 1) {
        QJsonObject firstPos;
        firstPos[QStringLiteral("x")] = normalized.x();
        firstPos[QStringLiteral("y")] = normalized.y();
        moveToNormalizedCenter(this, firstPos);
    }
    update();
    if (parentWidget()) {
        parentWidget()->update();
    }
    return true;
}

void MultiClickNodeWidget::setPlacementMode(bool enabled)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, enabled);
}

void MultiClickNodeWidget::paintGuide(QPainter &painter) const
{
    if (!parentWidget()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing);
    QPoint previous;
    for (int i = 0; i < m_clickPositions.size(); ++i) {
        const QPoint current(qRound(m_clickPositions.at(i).x() * parentWidget()->width()),
                             qRound(m_clickPositions.at(i).y() * parentWidget()->height()));
        if (i > 0) {
            painter.setPen(QPen(QColor(190, 145, 255, 180), 2, Qt::DashLine));
            painter.drawLine(previous, current);
        }
        painter.setPen(QPen(QColor(220, 195, 255), 2));
        painter.setBrush(QColor(120, 70, 205, 220));
        painter.drawEllipse(current, 11, 11);
        painter.setPen(Qt::white);
        painter.drawText(QRect(current.x() - 9, current.y() - 9, 18, 18),
                         Qt::AlignCenter, QString::number(i + 1));
        previous = current;
    }
}

void MultiClickNodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(190, 145, 255), 2));
    p.setBrush(QColor(85, 55, 145, 220));
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));
    p.setPen(Qt::white);
    QFont font = p.font();
    font.setPointSize(8);
    font.setBold(true);
    p.setFont(font);
    p.drawText(rect().adjusted(2, 8, -2, -2),
               Qt::AlignCenter, tr("MULTI ×%1\n%2").arg(m_clickPositions.size()).arg(m_keyName));
    paintCloseButton(p);
}

void MultiClickNodeWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint oldPos = pos();
    KeymapNodeWidget::mouseMoveEvent(event);
    const QPoint delta = pos() - oldPos;
    if (!delta.isNull() && parentWidget() && !parentWidget()->size().isEmpty()) {
        for (QPointF &position : m_clickPositions) {
            position.setX(qBound(0.0, position.x()
                + static_cast<double>(delta.x()) / parentWidget()->width(), 1.0));
            position.setY(qBound(0.0, position.y()
                + static_cast<double>(delta.y()) / parentWidget()->height(), 1.0));
        }
        parentWidget()->update();
    }
}

// ===========================================================================
// SteerWheelNodeWidget
// ===========================================================================

SteerWheelNodeWidget::SteerWheelNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    setFixedSize(120, 120);
    m_keyName = QStringLiteral("WASD");
}

QJsonObject SteerWheelNodeWidget::toJson() const
{
    QJsonObject obj = m_originalJson;
    obj[QStringLiteral("type")] = QStringLiteral("KMT_STEER_WHEEL");
    obj[QStringLiteral("centerPos")] = normalizedCenter(this);
    obj[QStringLiteral("leftKey")] = m_leftKey;
    obj[QStringLiteral("rightKey")] = m_rightKey;
    obj[QStringLiteral("upKey")] = m_upKey;
    obj[QStringLiteral("downKey")] = m_downKey;
    if (!obj.value(QStringLiteral("leftOffset")).isDouble()) obj[QStringLiteral("leftOffset")] = 0.05;
    if (!obj.value(QStringLiteral("rightOffset")).isDouble()) obj[QStringLiteral("rightOffset")] = 0.05;
    if (!obj.value(QStringLiteral("upOffset")).isDouble()) obj[QStringLiteral("upOffset")] = 0.05;
    if (!obj.value(QStringLiteral("downOffset")).isDouble()) obj[QStringLiteral("downOffset")] = 0.05;
    return obj;
}

void SteerWheelNodeWidget::fromJson(const QJsonObject &json)
{
    m_originalJson = json;
    if (json.value(QStringLiteral("centerPos")).isObject()) {
        moveToNormalizedCenter(this, json.value(QStringLiteral("centerPos")).toObject());
    }
    if (json.contains(QStringLiteral("upKey")))    m_upKey    = json[QStringLiteral("upKey")].toString();
    if (json.contains(QStringLiteral("downKey")))  m_downKey  = json[QStringLiteral("downKey")].toString();
    if (json.contains(QStringLiteral("leftKey")))  m_leftKey  = json[QStringLiteral("leftKey")].toString();
    if (json.contains(QStringLiteral("rightKey"))) m_rightKey = json[QStringLiteral("rightKey")].toString();
}

void SteerWheelNodeWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Don't open generic capture — open 4-key direction dialog
    if (event->button() != Qt::LeftButton || closeButtonRect().contains(event->pos())) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    // A small dialog with 4 capture buttons
    QDialog dlg(this);
    dlg.setWindowTitle(tr("WASD Keys — Double-click a direction to change"));
    dlg.setFixedSize(300, 240);

    auto *vlay = new QVBoxLayout(&dlg);
    vlay->addWidget(new QLabel(tr("Double-click a direction button to reassign it:"), &dlg));

    struct DirInfo { QString *key; const char *label; };
    DirInfo dirs[] = {
        { &m_upKey,    "Up  (↑)" },
        { &m_downKey,  "Down (↓)" },
        { &m_leftKey,  "Left (←)" },
        { &m_rightKey, "Right (→)" },
    };

    for (auto &d : dirs) {
        auto *row = new QHBoxLayout();
        auto *lbl = new QLabel(tr(d.label), &dlg);
        lbl->setFixedWidth(80);
        auto *btn = new QPushButton(*d.key, &dlg);
        btn->setFixedWidth(180);

        // Capture on click
        QString *keyPtr = d.key;
        QPushButton *btnPtr = btn;
        connect(btn, &QPushButton::clicked, [this, keyPtr, btnPtr, &dlg]() {
            KeyCaptureDlg kd(&dlg);
            if (kd.exec() == QDialog::Accepted && !kd.capturedKey().isEmpty()) {
                *keyPtr = kd.capturedKey();
                btnPtr->setText(*keyPtr);
                update();
            }
        });

        row->addWidget(lbl);
        row->addWidget(btn);
        vlay->addLayout(row);
    }

    auto *ok = new QPushButton(tr("Done"), &dlg);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    vlay->addWidget(ok);

    dlg.exec();
    event->accept();
}

void SteerWheelNodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Outer ring
    p.setPen(QPen(QColor(0, 230, 100), 2, Qt::DashLine));
    p.setBrush(QColor(0, 100, 40, 110));
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));

    // Crosshair
    p.setPen(QPen(QColor(0, 230, 100), 1));
    p.drawLine(width() / 2, 10, width() / 2, height() - 10);
    p.drawLine(10, height() / 2, width() - 10, height() / 2);

    // Direction labels (strip "Key_" prefix for display)
    auto shortKey = [](const QString &k) {
        return k.startsWith("Key_") ? k.mid(4) : k;
    };

    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPointSize(10);
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect().adjusted(0, 6, 0, 0),  Qt::AlignTop    | Qt::AlignHCenter, shortKey(m_upKey));
    p.drawText(rect().adjusted(0, 0, 0, -6), Qt::AlignBottom | Qt::AlignHCenter, shortKey(m_downKey));
    p.drawText(rect().adjusted(6, 0, 0, 0),  Qt::AlignLeft   | Qt::AlignVCenter, shortKey(m_leftKey));
    p.drawText(rect().adjusted(0, 0, -6, 0), Qt::AlignRight  | Qt::AlignVCenter, shortKey(m_rightKey));

    // "WASD" hint in center
    f.setPointSize(7);
    f.setBold(false);
    p.setFont(f);
    p.setPen(QColor(180, 255, 180, 160));
    p.drawText(rect(), Qt::AlignCenter, tr("dbl-click\nto edit"));

    paintCloseButton(p);
}

// ===========================================================================
// MouseMoveNodeWidget
// ===========================================================================

MouseMoveNodeWidget::MouseMoveNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    setFixedSize(150, 110);
    m_keyName = QStringLiteral("Camera Look");
}

void MouseMoveNodeWidget::setSwitchKey(const QString &key)
{
    m_switchKey = key;
    update();
}

QJsonObject MouseMoveNodeWidget::toJson() const
{
    QJsonObject obj = m_originalJson;
    if (!obj.value(QStringLiteral("speedRatio")).isDouble()) {
        obj.remove(QStringLiteral("speedRatio"));
    }
    obj[QStringLiteral("startPos")] = normalizedCenter(this);
    obj[QStringLiteral("speedRatioX")] = m_speedRatioX;
    obj[QStringLiteral("speedRatioY")] = m_speedRatioY;

    QJsonObject smallEyes = obj.value(QStringLiteral("smallEyes")).toObject();
    smallEyes[QStringLiteral("type")] = QStringLiteral("KMT_CLICK");
    smallEyes[QStringLiteral("key")] = m_switchKey;
    const QJsonValue eyePosition = smallEyes.value(QStringLiteral("pos"));
    if (!eyePosition.isObject()
        || !eyePosition.toObject().value(QStringLiteral("x")).isDouble()
        || !eyePosition.toObject().value(QStringLiteral("y")).isDouble()) {
        QJsonObject eyePos;
        eyePos[QStringLiteral("x")] = 0.5;
        eyePos[QStringLiteral("y")] = 0.5;
        smallEyes[QStringLiteral("pos")] = eyePos;
    }
    if (!smallEyes.value(QStringLiteral("switchMap")).isBool()) {
        smallEyes[QStringLiteral("switchMap")] = true;
    }
    obj[QStringLiteral("smallEyes")] = smallEyes;

    return obj;
}

void MouseMoveNodeWidget::fromJson(const QJsonObject &json)
{
    m_originalJson = json;
    if (json.value(QStringLiteral("startPos")).isObject()) {
        moveToNormalizedCenter(this, json.value(QStringLiteral("startPos")).toObject());
    }
    const bool hasLegacySpeed = json.value(QStringLiteral("speedRatio")).isDouble();
    const bool hasSpeedX = json.value(QStringLiteral("speedRatioX")).isDouble();
    const bool hasSpeedY = json.value(QStringLiteral("speedRatioY")).isDouble();
    const double defaultSpeed = hasLegacySpeed
        ? json.value(QStringLiteral("speedRatio")).toDouble()
        : 3.0;
    m_speedRatioX = json.value(QStringLiteral("speedRatioX")).toDouble(defaultSpeed);
    m_speedRatioY = json.value(QStringLiteral("speedRatioY")).toDouble(defaultSpeed / 2.25);
    if (!hasLegacySpeed && !hasSpeedX && hasSpeedY) {
        m_speedRatioX = m_speedRatioY * 2.25;
    }

    // Load switch key from smallEyes
    if (json.contains(QStringLiteral("smallEyes"))) {
        QJsonObject se = json[QStringLiteral("smallEyes")].toObject();
        if (se.contains(QStringLiteral("key")))
            m_switchKey = se[QStringLiteral("key")].toString();
    }
}

void MouseMoveNodeWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || closeButtonRect().contains(event->pos())) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    // Dialog to reassign toggle key
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Camera Look Settings"));
    dlg.setFixedSize(360, 250);

    auto *vlay = new QVBoxLayout(&dlg);
    vlay->addWidget(new QLabel(tr("Toggle Key (shows/hides mouse cursor):"), &dlg));

    QString switchKey = m_switchKey;
    auto *switchBtn = new QPushButton(switchKey, &dlg);
    switchBtn->setFixedHeight(40);
    vlay->addWidget(switchBtn);

    vlay->addWidget(new QLabel(tr("Click button to capture new toggle key."), &dlg));

    auto *speedXRow = new QHBoxLayout();
    speedXRow->addWidget(new QLabel(tr("Horizontal speed:"), &dlg));
    auto *speedX = new QDoubleSpinBox(&dlg);
    speedX->setRange(0.001, qMax(1000.0, m_speedRatioX));
    speedX->setSingleStep(0.1);
    speedX->setDecimals(3);
    speedX->setValue(m_speedRatioX);
    speedXRow->addWidget(speedX);
    vlay->addLayout(speedXRow);

    auto *speedYRow = new QHBoxLayout();
    speedYRow->addWidget(new QLabel(tr("Vertical speed:"), &dlg));
    auto *speedY = new QDoubleSpinBox(&dlg);
    speedY->setRange(0.001, qMax(1000.0, m_speedRatioY));
    speedY->setSingleStep(0.1);
    speedY->setDecimals(3);
    speedY->setValue(m_speedRatioY);
    speedYRow->addWidget(speedY);
    vlay->addLayout(speedYRow);

    connect(switchBtn, &QPushButton::clicked, [switchBtn, &dlg, &switchKey]() {
        KeyCaptureDlg kd(&dlg);
        if (kd.exec() == QDialog::Accepted && !kd.capturedKey().isEmpty()) {
            switchKey = kd.capturedKey();
            switchBtn->setText(switchKey);
        }
    });

    auto *ok = new QPushButton(tr("Done"), &dlg);
    connect(ok, &QPushButton::clicked, &dlg, [&dlg, speedX, speedY, &switchKey, this]() {
        m_switchKey = switchKey;
        m_speedRatioX = speedX->value();
        m_speedRatioY = speedY->value();
        dlg.accept();
    });
    vlay->addWidget(ok);

    dlg.exec();
    update();
    event->accept();
}

void MouseMoveNodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.setPen(QPen(QColor(100, 150, 255), 2));
    p.setBrush(QColor(30, 60, 180, 120));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);

    // Title
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(10);
    p.setFont(f);
    p.drawText(QRect(0, 10, width(), 22), Qt::AlignCenter, tr("🎯 Camera Look"));

    // Toggle key info
    f.setPointSize(8);
    f.setBold(false);
    p.setFont(f);
    p.setPen(QColor(180, 210, 255));

    QString shortSwitch = m_switchKey.startsWith("Key_") ? m_switchKey.mid(4) : m_switchKey;
    p.drawText(QRect(0, 38, width(), 20), Qt::AlignCenter,
               tr("Toggle: ") + shortSwitch);
    p.drawText(QRect(0, 58, width(), 20), Qt::AlignCenter,
               tr("X ×%1  Y ×%2").arg(m_speedRatioX, 0, 'f', 1)
                                      .arg(m_speedRatioY, 0, 'f', 1));

    // Help hint
    p.setPen(QColor(140, 170, 255, 160));
    f.setPointSize(7);
    p.setFont(f);
    p.drawText(QRect(0, 80, width(), 20), Qt::AlignCenter, tr("dbl-click to configure"));

    paintCloseButton(p);
}