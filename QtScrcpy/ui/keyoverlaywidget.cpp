#include <QEvent>
#include <QFontMetrics>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineF>
#include <QPainter>
#include <QPaintEvent>
#include <cmath>

#include "keyoverlaywidget.h"

KeyOverlayWidget::KeyOverlayWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    if (parent) {
        parent->installEventFilter(this);
    }
}

bool KeyOverlayWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
    }
    return QWidget::eventFilter(watched, event);
}

bool KeyOverlayWidget::readPosition(const QJsonObject &object, const QString &key, QPointF &position)
{
    const QJsonValue value = object.value(key);
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject point = value.toObject();
    if (!point.value("x").isDouble() || !point.value("y").isDouble()) {
        return false;
    }

    const qreal x = point.value("x").toDouble();
    const qreal y = point.value("y").toDouble();
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return false;
    }

    position = QPointF(x, y);
    return true;
}

QString KeyOverlayWidget::displayKey(const QString &key)
{
    if (key == "LeftButton") return "LMB";
    if (key == "RightButton") return "RMB";
    if (key == "MiddleButton") return "MMB";

    QString label = key;
    if (label.startsWith("Key_")) {
        label.remove(0, 4);
    }

    if (label == "Space") return "SPACE";
    if (label == "Shift") return "SHIFT";
    if (label == "Control") return "CTRL";
    if (label == "Alt") return "ALT";
    if (label == "Meta") return "WIN";
    if (label == "Tab") return "TAB";
    if (label == "Escape") return "ESC";
    if (label == "Backspace") return "BACK";
    if (label == "Return" || label == "Enter") return "ENTER";
    if (label == "QuoteLeft") return "~";
    if (label == "Left") return QString::fromUtf8("\xe2\x86\x90");
    if (label == "Right") return QString::fromUtf8("\xe2\x86\x92");
    if (label == "Up") return QString::fromUtf8("\xe2\x86\x91");
    if (label == "Down") return QString::fromUtf8("\xe2\x86\x93");
    if (label == "Equal") return "=";

    return label.toUpper();
}

void KeyOverlayWidget::addMarker(const QPointF &position, const QString &key)
{
    if (key.isEmpty() || position.x() < 0.0 || position.x() > 1.0
        || position.y() < 0.0 || position.y() > 1.0) {
        return;
    }

    Marker marker;
    marker.position = position;
    marker.label = displayKey(key);
    if (!marker.label.isEmpty()) {
        m_markers.push_back(marker);
    }
}

bool KeyOverlayWidget::setScript(const QString &script)
{
    m_markers.clear();
    const QJsonDocument document = QJsonDocument::fromJson(script.toUtf8());
    if (!document.isObject()) {
        update();
        return false;
    }

    const QJsonObject root = document.object();
    const QJsonArray nodes = root.value("keyMapNodes").toArray();
    for (const QJsonValue &value : nodes) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject node = value.toObject();
        const QString type = node.value("type").toString();
        const QString key = node.value("key").toString();
        QPointF position;

        if (type == "KMT_CLICK" || type == "KMT_CLICK_TWICE") {
            if (readPosition(node, "pos", position)) {
                addMarker(position, key);
            }
        } else if (type == "KMT_CLICK_MULTI") {
            const QJsonArray clickNodes = node.value("clickNodes").toArray();
            for (const QJsonValue &clickValue : clickNodes) {
                if (!clickValue.isObject()) {
                    continue;
                }
                if (readPosition(clickValue.toObject(), "pos", position)) {
                    addMarker(position, key);
                }
            }
        } else if (type == "KMT_STEER_WHEEL") {
            QPointF center;
            if (!readPosition(node, "centerPos", center)) {
                continue;
            }

            const qreal leftOffset = node.value("leftOffset").toDouble(0.08);
            const qreal rightOffset = node.value("rightOffset").toDouble(0.08);
            const qreal upOffset = node.value("upOffset").toDouble(0.08);
            const qreal downOffset = node.value("downOffset").toDouble(0.08);
            addMarker(QPointF(center.x() - leftOffset, center.y()), node.value("leftKey").toString());
            addMarker(QPointF(center.x() + rightOffset, center.y()), node.value("rightKey").toString());
            addMarker(QPointF(center.x(), center.y() - upOffset), node.value("upKey").toString());
            addMarker(QPointF(center.x(), center.y() + downOffset), node.value("downKey").toString());
        } else if (type == "KMT_DRAG") {
            QPointF endPosition;
            if (!key.isEmpty() && readPosition(node, "startPos", position)
                && readPosition(node, "endPos", endPosition)) {
                addMarker(position, key);
                Marker &marker = m_markers.last();
                marker.endPosition = endPosition;
                marker.hasEndPosition = true;
            }
        }
    }

    const QJsonObject mouseMoveMap = root.value("mouseMoveMap").toObject();
    QPointF position;
    if (readPosition(mouseMoveMap, "startPos", position)) {
        addMarker(position, "MOUSE");
    }
    const QJsonObject smallEyes = mouseMoveMap.value("smallEyes").toObject();
    if (readPosition(smallEyes, "pos", position)) {
        addMarker(position, smallEyes.value("key").toString());
    }

    update();
    return !m_markers.isEmpty();
}

void KeyOverlayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(qBound(11, width() / 55, 18));
    painter.setFont(font);
    const QFontMetrics metrics(font);

    const QRectF bounds = rect();
    const qreal badgeHeight = metrics.height() + 8.0;
    for (const Marker &marker : m_markers) {
        if (marker.position.x() < 0.0 || marker.position.x() > 1.0
            || marker.position.y() < 0.0 || marker.position.y() > 1.0) {
            continue;
        }

        const QPointF point(marker.position.x() * width(), marker.position.y() * height());
        if (marker.hasEndPosition
            && marker.endPosition.x() >= 0.0 && marker.endPosition.x() <= 1.0
            && marker.endPosition.y() >= 0.0 && marker.endPosition.y() <= 1.0) {
            const QPointF endPoint(marker.endPosition.x() * width(), marker.endPosition.y() * height());
            painter.setPen(QPen(QColor(72, 205, 255, 205), 2.0, Qt::DashLine));
            painter.drawLine(point, endPoint);

            QLineF direction(point, endPoint);
            if (direction.length() > 1.0) {
                const qreal lineLength = direction.length();
                QPointF arrowTip = endPoint;
                if (lineLength > 8.0) {
                    direction.setLength(lineLength - 8.0);
                    arrowTip = direction.p2();
                }
                const qreal angle = std::atan2(endPoint.y() - point.y(), endPoint.x() - point.x());
                const qreal arrowLength = 7.0;
                const QPointF arrowLeft(arrowTip.x() - arrowLength * std::cos(angle - 0.55),
                                        arrowTip.y() - arrowLength * std::sin(angle - 0.55));
                const QPointF arrowRight(arrowTip.x() - arrowLength * std::cos(angle + 0.55),
                                         arrowTip.y() - arrowLength * std::sin(angle + 0.55));
                painter.drawLine(arrowTip, arrowLeft);
                painter.drawLine(arrowTip, arrowRight);
            }
        }

        const qreal badgeWidth = qMax(badgeHeight + 2.0, metrics.horizontalAdvance(marker.label) + 16.0);
        QRectF badge(point.x() - badgeWidth / 2.0, point.y() - badgeHeight / 2.0, badgeWidth, badgeHeight);
        badge.moveLeft(qBound(bounds.left(), badge.left(), bounds.right() - badge.width()));
        badge.moveTop(qBound(bounds.top(), badge.top(), bounds.bottom() - badge.height()));

        painter.setPen(QPen(QColor(72, 205, 255, 235), 1.5));
        painter.setBrush(QColor(7, 15, 25, 205));
        painter.drawRoundedRect(badge, badgeHeight / 2.0, badgeHeight / 2.0);
        painter.setPen(Qt::white);
        painter.drawText(badge, Qt::AlignCenter, marker.label);
    }
}
