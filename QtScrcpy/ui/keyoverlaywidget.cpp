#include <QEvent>
#include <QFontMetrics>
#include <QCursor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLineF>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QSlider>
#include <QToolTip>
#include <QWidget>
#include <cmath>

#include "keyoverlaywidget.h"

KeyOverlayWidget::KeyOverlayWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    installEventFilter(this);
    if (parent) {
        parent->installEventFilter(this);
    }

    m_editToolbar = new QWidget(this);
    m_editToolbar->setObjectName(QStringLiteral("keymapEditToolbar"));
    m_editToolbar->setStyleSheet(
        "QWidget#keymapEditToolbar { background: #17212b; border: 1px solid #48cdff; border-radius: 5px; }"
        "QLabel { color: white; border: none; background: transparent; }"
        "QPushButton { color: white; background: #263746; border: 1px solid #5b7184; border-radius: 3px; padding: 3px 7px; }"
        "QPushButton:hover { background: #34516a; }"
    );
    auto *layout = new QHBoxLayout(m_editToolbar);
    layout->setContentsMargins(6, 3, 6, 3);
    layout->setSpacing(5);
    m_editHint = new QLabel(tr("Press a key • drag or click to place"), m_editToolbar);
    m_editHint->setToolTip(tr("Press a mapped key to select its marker, then drag or click to move it. Press an unused key and click or drag to add it. Use Aa to set key size and opacity, the tick to save, and the X to cancel. Right-click a marker to change or remove it."));
    layout->addWidget(m_editHint, 1);
    m_styleButton = new QPushButton(tr("Aa"), m_editToolbar);
    m_styleButton->setToolTip(tr("Edit key size and opacity"));
    m_styleButton->setAccessibleName(tr("Edit key size and opacity"));
    m_styleButton->setFixedSize(38, 30);
    layout->addWidget(m_styleButton);
    m_saveButton = new QPushButton(QString::fromUtf8("\xe2\x9c\x93"), m_editToolbar);
    m_saveButton->setToolTip(tr("Save keymap"));
    m_saveButton->setAccessibleName(tr("Save keymap"));
    m_saveButton->setFixedSize(30, 30);
    m_cancelButton = new QPushButton(QString::fromUtf8("\xe2\x9c\x95"), m_editToolbar);
    m_cancelButton->setToolTip(tr("Cancel editing"));
    m_cancelButton->setAccessibleName(tr("Cancel editing"));
    m_cancelButton->setFixedSize(30, 30);
    layout->addWidget(m_saveButton);
    layout->addWidget(m_cancelButton);
    m_editToolbar->hide();

    connect(m_saveButton, &QPushButton::clicked, this, &KeyOverlayWidget::requestSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &KeyOverlayWidget::cancelEditing);
    connect(m_styleButton, &QPushButton::clicked, this, &KeyOverlayWidget::editAppearanceDialog);
}

bool KeyOverlayWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this && event->type() == QEvent::KeyPress && m_editMode) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->isAutoRepeat()) {
            return true;
        }
        if (keyEvent->key() == Qt::Key_Escape) {
            m_pendingKey.clear();
            m_selectedMarkerIndex = -1;
            m_placingPendingKey = false;
            m_draggingMarker = -1;
            m_editHint->setText(tr("Press a key • drag or click to place"));
            setCursor(Qt::CrossCursor);
            update();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Shift || keyEvent->key() == Qt::Key_Control
            || keyEvent->key() == Qt::Key_Alt || keyEvent->key() == Qt::Key_Meta) {
            return true;
        }
        if (keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
            m_editHint->setText(tr("Use one key without Ctrl, Alt, or Meta"));
            return true;
        }

        const char *enumName = QMetaEnum::fromType<Qt::Key>().valueToKey(keyEvent->key());
        QString key = enumName ? QString::fromLatin1(enumName) : QString();
        if (key == QLatin1String("Key_AsciiTilde")) {
            key = QStringLiteral("Key_QuoteLeft");
        }
        if (key.isEmpty() || key == QLatin1String("Key_unknown")) {
            m_editHint->setText(tr("That key is not supported; press another key"));
            return true;
        }
        const int existingMarker = editableMarkerForKey(key);
        if (existingMarker >= 0) {
            m_selectedMarkerIndex = existingMarker;
            m_pendingKey = key;
            m_placingPendingKey = false;
            m_draggingMarker = -1;
            m_editHint->setText(tr("%1 selected — drag or click where it should go").arg(displayKey(key)));
            update();
            return true;
        }
        if (keyIsUsed(key)) {
            m_pendingKey.clear();
            m_selectedMarkerIndex = -1;
            m_editHint->setText(tr("%1 is already used and has no movable marker").arg(displayKey(key)));
            return true;
        }

        m_selectedMarkerIndex = -1;
        m_placingPendingKey = false;
        m_pendingKey = key;
        m_editHint->setText(tr("%1 selected — move or drag, then click to place (Esc cancels)").arg(displayKey(key)));
        update();
        return true;
    }
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
        updateEditToolbarGeometry();
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

QString KeyOverlayWidget::parseKeyName(const QString &keyText)
{
    const QKeySequence sequence = QKeySequence::fromString(keyText.trimmed(), QKeySequence::PortableText);
    if (sequence.count() != 1) {
        return QString();
    }

    const int sequenceValue = sequence[0];
    if (sequenceValue & Qt::KeyboardModifierMask) {
        return QString();
    }

    const char *keyName = QMetaEnum::fromType<Qt::Key>().valueToKey(sequenceValue);
    return keyName ? QString::fromLatin1(keyName) : QString();
}

QJsonObject KeyOverlayWidget::toJsonPosition(const QPointF &position)
{
    return QJsonObject {
        { QStringLiteral("x"), qBound(0.0, position.x(), 1.0) },
        { QStringLiteral("y"), qBound(0.0, position.y(), 1.0) }
    };
}

void KeyOverlayWidget::addMarker(const QPointF &position, const QString &key)
{
    if (key.isEmpty() || position.x() < 0.0 || position.x() > 1.0
        || position.y() < 0.0 || position.y() > 1.0) {
        return;
    }

    Marker marker;
    marker.position = position;
    marker.key = key;
    marker.label = displayKey(key);
    if (!marker.label.isEmpty()) {
        m_markers.push_back(marker);
    }
}

void KeyOverlayWidget::addNodeMarker(const QPointF &position, const QString &key, int nodeIndex,
                                     const QString &positionField, PositionTarget target,
                                     int childIndex, const QString &keyField)
{
    if (key.isEmpty() || position.x() < 0.0 || position.x() > 1.0
        || position.y() < 0.0 || position.y() > 1.0) {
        return;
    }

    Marker marker;
    marker.position = position;
    marker.key = key;
    marker.label = displayKey(key);
    marker.nodeIndex = nodeIndex;
    marker.childIndex = childIndex;
    marker.positionField = positionField;
    marker.keyField = keyField;
    marker.target = target;
    if (!marker.label.isEmpty()) {
        m_markers.push_back(marker);
    }
}

bool KeyOverlayWidget::setScript(const QString &script)
{
    const bool restoreWindowFocus = m_editMode;
    m_editMode = false;
    m_pendingKey.clear();
    m_selectedMarkerIndex = -1;
    m_placingPendingKey = false;
    m_editHint->setText(tr("Press a key • drag or click to place"));
    m_editToolbar->hide();
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);
    unsetCursor();
    m_draggingMarker = -1;
    m_originalScript = script;
    m_markers.clear();
    m_root = QJsonObject();
    loadAppearanceSettings();

    const QJsonDocument document = QJsonDocument::fromJson(script.toUtf8());
    m_validScript = document.isObject() && document.object().value("keyMapNodes").isArray();
    if (!m_validScript) {
        update();
        if (restoreWindowFocus && window() && window() != this) {
            window()->setFocus(Qt::OtherFocusReason);
        }
        return false;
    }

    m_root = document.object();
    loadAppearanceSettings();
    rebuildMarkers();
    if (restoreWindowFocus && window() && window() != this) {
        window()->setFocus(Qt::OtherFocusReason);
    }
    return true;
}

bool KeyOverlayWidget::setEditMode(bool enabled)
{
    if (enabled && !m_validScript) {
        return false;
    }

    m_editMode = enabled;
    if (!enabled) {
        m_pendingKey.clear();
        m_selectedMarkerIndex = -1;
        m_placingPendingKey = false;
    } else {
        m_editHint->setText(tr("Press a key • drag or click to place"));
    }
    m_draggingMarker = -1;
    setAttribute(Qt::WA_TransparentForMouseEvents, !enabled);
    if (enabled) {
        setFocusPolicy(Qt::StrongFocus);
        setFocus(Qt::MouseFocusReason);
        setCursor(Qt::CrossCursor);
        updateEditToolbarGeometry();
        m_editToolbar->show();
        m_editToolbar->raise();
    } else {
        m_editToolbar->hide();
        setFocusPolicy(Qt::NoFocus);
        unsetCursor();
        if (window() && window() != this) {
            window()->setFocus(Qt::OtherFocusReason);
        }
    }
    update();
    return true;
}

void KeyOverlayWidget::cancelEditing()
{
    if (!m_editMode) {
        return;
    }
    const QJsonDocument original = QJsonDocument::fromJson(m_originalScript.toUtf8());
    if (original.isObject()) {
        m_root = original.object();
        loadAppearanceSettings();
        rebuildMarkers();
    }
    m_pendingKey.clear();
    m_editHint->setText(tr("Press a key • drag or click to place"));
    setEditMode(false);
}

void KeyOverlayWidget::requestSave()
{
    if (m_editMode && m_validScript) {
        emit saveRequested(serializedScript());
    }
}

void KeyOverlayWidget::loadAppearanceSettings()
{
    m_keySize = 1.0;
    m_keyOpacity = 1.0;

    const QJsonObject appearance = m_root.value("keyOverlayStyle").toObject();
    const QJsonValue keySize = appearance.value("keySize");
    if (keySize.isDouble() && std::isfinite(keySize.toDouble())) {
        m_keySize = qBound(0.5, keySize.toDouble(), 2.0);
    }

    const QJsonValue opacity = appearance.value("opacity");
    if (opacity.isDouble() && std::isfinite(opacity.toDouble())) {
        m_keyOpacity = qBound(0.2, opacity.toDouble(), 1.0);
    }
}

void KeyOverlayWidget::editAppearanceDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Key appearance"));
    dialog.setMinimumWidth(320);

    auto *layout = new QFormLayout(&dialog);
    auto *sizeSlider = new QSlider(Qt::Horizontal, &dialog);
    sizeSlider->setRange(50, 200);
    sizeSlider->setValue(qRound(m_keySize * 100.0));
    auto *sizeValue = new QLabel(&dialog);
    auto *sizeRow = new QWidget(&dialog);
    auto *sizeLayout = new QHBoxLayout(sizeRow);
    sizeLayout->setContentsMargins(0, 0, 0, 0);
    sizeLayout->addWidget(sizeSlider, 1);
    sizeLayout->addWidget(sizeValue);
    layout->addRow(tr("Key size"), sizeRow);

    auto *opacitySlider = new QSlider(Qt::Horizontal, &dialog);
    opacitySlider->setRange(20, 100);
    opacitySlider->setValue(qRound(m_keyOpacity * 100.0));
    auto *opacityValue = new QLabel(&dialog);
    auto *opacityRow = new QWidget(&dialog);
    auto *opacityLayout = new QHBoxLayout(opacityRow);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    opacityLayout->addWidget(opacitySlider, 1);
    opacityLayout->addWidget(opacityValue);
    layout->addRow(tr("Opacity"), opacityRow);

    auto updateSizeLabel = [sizeValue](int value) {
        sizeValue->setText(QStringLiteral("%1%").arg(value));
    };
    auto updateOpacityLabel = [opacityValue](int value) {
        opacityValue->setText(QStringLiteral("%1%").arg(value));
    };
    connect(sizeSlider, &QSlider::valueChanged, &dialog, updateSizeLabel);
    connect(opacitySlider, &QSlider::valueChanged, &dialog, updateOpacityLabel);
    updateSizeLabel(sizeSlider->value());
    updateOpacityLabel(opacitySlider->value());

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_keySize = sizeSlider->value() / 100.0;
    m_keyOpacity = opacitySlider->value() / 100.0;
    QJsonObject appearance = m_root.value("keyOverlayStyle").toObject();
    appearance.insert(QStringLiteral("keySize"), m_keySize);
    appearance.insert(QStringLiteral("opacity"), m_keyOpacity);
    m_root.insert(QStringLiteral("keyOverlayStyle"), appearance);
    update();
}

void KeyOverlayWidget::rebuildMarkers()
{
    m_markers.clear();
    const QJsonArray nodes = m_root.value("keyMapNodes").toArray();
    for (int nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
        const QJsonValue value = nodes.at(nodeIndex);
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject node = value.toObject();
        const QString type = node.value("type").toString();
        const QString key = node.value("key").toString();
        QPointF position;

        if (type == "KMT_CLICK" || type == "KMT_CLICK_TWICE") {
            if (readPosition(node, "pos", position)) {
                addNodeMarker(position, key, nodeIndex, "pos");
            }
        } else if (type == "KMT_CLICK_MULTI") {
            const QJsonArray clickNodes = node.value("clickNodes").toArray();
            for (int clickIndex = 0; clickIndex < clickNodes.size(); ++clickIndex) {
                const QJsonValue clickValue = clickNodes.at(clickIndex);
                if (clickValue.isObject() && readPosition(clickValue.toObject(), "pos", position)) {
                    addNodeMarker(position, key, nodeIndex, "pos", PositionTarget::MultiClickField, clickIndex);
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
            auto addDirectionMarker = [this, nodeIndex](const QPointF &point, const QString &key,
                                                        const QString &direction, const QString &offset) {
                const int previousCount = m_markers.size();
                addNodeMarker(point, key, nodeIndex, direction, PositionTarget::SteeringDirection,
                              -1, direction + QStringLiteral("Key"));
                if (m_markers.size() > previousCount) {
                    m_markers.last().offsetField = offset;
                }
            };
            addDirectionMarker(QPointF(center.x() - leftOffset, center.y()), node.value("leftKey").toString(), "left", "leftOffset");
            addDirectionMarker(QPointF(center.x() + rightOffset, center.y()), node.value("rightKey").toString(), "right", "rightOffset");
            addDirectionMarker(QPointF(center.x(), center.y() - upOffset), node.value("upKey").toString(), "up", "upOffset");
            addDirectionMarker(QPointF(center.x(), center.y() + downOffset), node.value("downKey").toString(), "down", "downOffset");
        } else if (type == "KMT_DRAG") {
            QPointF endPosition;
            if (!key.isEmpty() && readPosition(node, "startPos", position)
                && readPosition(node, "endPos", endPosition)) {
                addNodeMarker(position, key, nodeIndex, "startPos");
                Marker &marker = m_markers.last();
                marker.endPosition = endPosition;
                marker.endPositionField = "endPos";
                marker.hasEndPosition = true;
            }
        }
    }

    const QJsonObject mouseMoveMap = m_root.value("mouseMoveMap").toObject();
    QPointF position;
    if (readPosition(mouseMoveMap, "startPos", position)) {
        addNodeMarker(position, "MOUSE", -1, "startPos", PositionTarget::MouseMoveStart);
    }
    const QJsonObject smallEyes = mouseMoveMap.value("smallEyes").toObject();
    if (readPosition(smallEyes, "pos", position)) {
        addNodeMarker(position, smallEyes.value("key").toString(), -1, "pos",
                      PositionTarget::SmallEyesPosition, -1, "key");
    }

    update();
}

void KeyOverlayWidget::updateEditToolbarGeometry()
{
    if (!m_editToolbar) {
        return;
    }
    const int toolbarWidth = qMax(0, qMin(width() - 12, 430));
    m_editToolbar->setGeometry((width() - toolbarWidth) / 2, 6, toolbarWidth, 38);
}

int KeyOverlayWidget::markerAt(const QPointF &position, bool *atEnd) const
{
    const qreal hitRadius = qMax(14.0, qMin(width(), height()) / 24.0) * m_keySize;
    for (int index = m_markers.size() - 1; index >= 0; --index) {
        const Marker &marker = m_markers.at(index);
        const QPointF point(marker.position.x() * width(), marker.position.y() * height());
        if (QLineF(position, point).length() <= hitRadius) {
            if (atEnd) *atEnd = false;
            return index;
        }
        if (marker.hasEndPosition) {
            const QPointF endPoint(marker.endPosition.x() * width(), marker.endPosition.y() * height());
            if (QLineF(position, endPoint).length() <= hitRadius) {
                if (atEnd) *atEnd = true;
                return index;
            }
        }
    }
    return -1;
}

int KeyOverlayWidget::editableMarkerForKey(const QString &key) const
{
    for (int index = 0; index < m_markers.size(); ++index) {
        const Marker &marker = m_markers.at(index);
        if (marker.key == key && marker.target != PositionTarget::ReadOnly
            && marker.target != PositionTarget::MouseMoveStart) {
            return index;
        }
    }
    return -1;
}

bool KeyOverlayWidget::keyIsUsed(const QString &key, int exceptNodeIndex) const
{
    if (m_root.value("switchKey").toString() == key) {
        return true;
    }
    const QJsonArray nodes = m_root.value("keyMapNodes").toArray();
    const QStringList directionKeys { "leftKey", "rightKey", "upKey", "downKey" };
    for (int index = 0; index < nodes.size(); ++index) {
        if (index == exceptNodeIndex || !nodes.at(index).isObject()) {
            continue;
        }
        const QJsonObject node = nodes.at(index).toObject();
        if (node.value("key").toString() == key) {
            return true;
        }
        for (const QString &field : directionKeys) {
            if (node.value(field).toString() == key) {
                return true;
            }
        }
    }
    const QJsonObject smallEyes = m_root.value("mouseMoveMap").toObject().value("smallEyes").toObject();
    return smallEyes.value("key").toString() == key;
}

bool KeyOverlayWidget::setMarkerPosition(int markerIndex, bool endPoint, const QPointF &position)
{
    if (markerIndex < 0 || markerIndex >= m_markers.size()) {
        return false;
    }
    Marker &marker = m_markers[markerIndex];
    if (marker.target == PositionTarget::ReadOnly
        || (endPoint && !marker.hasEndPosition)) {
        return false;
    }

    const QPointF normalized(qBound(0.0, position.x() / qMax(1, width()), 1.0),
                             qBound(0.0, position.y() / qMax(1, height()), 1.0));
    const QString field = endPoint ? marker.endPositionField : marker.positionField;

    if (marker.target == PositionTarget::MouseMoveStart) {
        QJsonObject mouseMoveMap = m_root.value("mouseMoveMap").toObject();
        mouseMoveMap.insert(field, toJsonPosition(normalized));
        m_root.insert("mouseMoveMap", mouseMoveMap);
    } else if (marker.target == PositionTarget::SmallEyesPosition) {
        QJsonObject mouseMoveMap = m_root.value("mouseMoveMap").toObject();
        QJsonObject smallEyes = mouseMoveMap.value("smallEyes").toObject();
        smallEyes.insert(field, toJsonPosition(normalized));
        mouseMoveMap.insert("smallEyes", smallEyes);
        m_root.insert("mouseMoveMap", mouseMoveMap);
    } else if (marker.nodeIndex < 0) {
        return false;
    }

    if (marker.target == PositionTarget::MouseMoveStart
        || marker.target == PositionTarget::SmallEyesPosition) {
        marker.position = normalized;
        update();
        return true;
    }

    QJsonArray nodes = m_root.value("keyMapNodes").toArray();
    if (marker.nodeIndex >= nodes.size() || !nodes.at(marker.nodeIndex).isObject()) {
        return false;
    }

    QJsonObject node = nodes.at(marker.nodeIndex).toObject();
    if (marker.target == PositionTarget::SteeringDirection) {
        QPointF center;
        if (!readPosition(node, "centerPos", center)) {
            return false;
        }
        if (marker.positionField == "left") {
            center.setY(normalized.y());
            node.insert(marker.offsetField, qMax(0.0, center.x() - normalized.x()));
        } else if (marker.positionField == "right") {
            center.setY(normalized.y());
            node.insert(marker.offsetField, qMax(0.0, normalized.x() - center.x()));
        } else if (marker.positionField == "up") {
            center.setX(normalized.x());
            node.insert(marker.offsetField, qMax(0.0, center.y() - normalized.y()));
        } else if (marker.positionField == "down") {
            center.setX(normalized.x());
            node.insert(marker.offsetField, qMax(0.0, normalized.y() - center.y()));
        }
        node.insert("centerPos", toJsonPosition(center));
    } else if (marker.target == PositionTarget::MultiClickField) {
        QJsonArray clickNodes = node.value("clickNodes").toArray();
        if (marker.childIndex < 0 || marker.childIndex >= clickNodes.size()
            || !clickNodes.at(marker.childIndex).isObject()) {
            return false;
        }
        QJsonObject clickNode = clickNodes.at(marker.childIndex).toObject();
        clickNode.insert(field, toJsonPosition(normalized));
        clickNodes.replace(marker.childIndex, clickNode);
        node.insert("clickNodes", clickNodes);
    } else {
        node.insert(field, toJsonPosition(normalized));
    }
    nodes.replace(marker.nodeIndex, node);
    m_root.insert("keyMapNodes", nodes);

    if (endPoint) {
        marker.endPosition = normalized;
    } else {
        marker.position = normalized;
    }
    if (marker.target == PositionTarget::SteeringDirection) {
        rebuildMarkers();
        return true;
    }
    update();
    return true;
}

bool KeyOverlayWidget::setMarkerKey(int markerIndex, const QString &key)
{
    if (markerIndex < 0 || markerIndex >= m_markers.size()) {
        return false;
    }
    Marker &marker = m_markers[markerIndex];
    if (marker.target == PositionTarget::ReadOnly || marker.target == PositionTarget::MouseMoveStart) {
        return false;
    }

    if (marker.target == PositionTarget::SmallEyesPosition) {
        QJsonObject mouseMoveMap = m_root.value("mouseMoveMap").toObject();
        QJsonObject smallEyes = mouseMoveMap.value("smallEyes").toObject();
        smallEyes.insert(marker.keyField, key);
        mouseMoveMap.insert("smallEyes", smallEyes);
        m_root.insert("mouseMoveMap", mouseMoveMap);
        marker.key = key;
        marker.label = displayKey(key);
        update();
        return true;
    }
    if (marker.nodeIndex < 0) {
        return false;
    }

    QJsonArray nodes = m_root.value("keyMapNodes").toArray();
    if (marker.nodeIndex >= nodes.size() || !nodes.at(marker.nodeIndex).isObject()) {
        return false;
    }
    QJsonObject node = nodes.at(marker.nodeIndex).toObject();
    node.insert(marker.keyField, key);
    nodes.replace(marker.nodeIndex, node);
    m_root.insert("keyMapNodes", nodes);

    const QString label = displayKey(key);
    for (Marker &related : m_markers) {
        if (related.nodeIndex == marker.nodeIndex && related.keyField == marker.keyField) {
            related.key = key;
            related.label = label;
        }
    }
    update();
    return true;
}

void KeyOverlayWidget::addMappingAt(const QPointF &position)
{
    QString key = m_pendingKey;
    if (key.isEmpty()) {
        bool accepted = false;
        const QString keyText = QInputDialog::getText(this, tr("Add key mapping"),
                                                      tr("Keyboard key (for example A, Space, or F1):"),
                                                      QLineEdit::Normal, QString(), &accepted);
        if (!accepted) {
            return;
        }
        key = parseKeyName(keyText);
    }
    if (key.isEmpty()) {
        QMessageBox::information(this, tr("Unsupported key"),
                                 tr("Enter one keyboard key such as A, Space, Enter, or F1. Modifier combinations are not supported by this keymap format."));
        return;
    }
    if (keyIsUsed(key)) {
        QMessageBox::information(this, tr("Key already mapped"),
                                 tr("That key is already used in this keymap. Choose a different key."));
        m_pendingKey.clear();
        m_editHint->setText(tr("That key is already mapped and cannot be added twice"));
        return;
    }

    QJsonArray nodes = m_root.value("keyMapNodes").toArray();
    QJsonObject mapping {
        { QStringLiteral("comment"), tr("Added in visual keymap editor") },
        { QStringLiteral("type"), QStringLiteral("KMT_CLICK") },
        { QStringLiteral("key"), key },
        { QStringLiteral("pos"), toJsonPosition(QPointF(position.x() / qMax(1, width()),
                                                         position.y() / qMax(1, height()))) },
        { QStringLiteral("switchMap"), false }
    };
    nodes.append(mapping);
    m_root.insert("keyMapNodes", nodes);
    m_pendingKey.clear();
    m_selectedMarkerIndex = -1;
    m_placingPendingKey = false;
    m_editHint->setText(tr("Press a key • drag or click to place"));
    rebuildMarkers();
}

void KeyOverlayWidget::editMarker(int markerIndex, const QPoint &globalPosition)
{
    if (markerIndex < 0 || markerIndex >= m_markers.size()) {
        return;
    }
    const Marker marker = m_markers.at(markerIndex);
    if (marker.target == PositionTarget::ReadOnly || marker.target == PositionTarget::MouseMoveStart) {
        QToolTip::showText(globalPosition, tr("Drag this marker to change its position. Edit advanced options in the JSON editor."), this);
        return;
    }

    QMenu menu(this);
    QAction *changeKey = nullptr;
    if (marker.target != PositionTarget::MouseMoveStart) {
        changeKey = menu.addAction(tr("Change key…"));
    }
    QAction *removeMapping = nullptr;
    if (marker.target == PositionTarget::NodeField || marker.target == PositionTarget::MultiClickField) {
        removeMapping = menu.addAction(marker.target == PositionTarget::MultiClickField
                                           ? tr("Remove this touch") : tr("Delete mapping"));
    }
    QAction *chosen = menu.exec(globalPosition);
    if (chosen == changeKey) {
        bool accepted = false;
        const QString keyText = QInputDialog::getText(this, tr("Change mapped key"),
                                                       tr("Keyboard key:"), QLineEdit::Normal,
                                                       marker.label, &accepted);
        if (!accepted) return;
        const QString key = parseKeyName(keyText);
        if (key.isEmpty()) {
            QMessageBox::information(this, tr("Unsupported key"), tr("Enter one keyboard key such as A, Space, Enter, or F1."));
            return;
        }
        QString currentKey;
        if (marker.target == PositionTarget::SmallEyesPosition) {
            currentKey = m_root.value("mouseMoveMap").toObject().value("smallEyes").toObject()
                             .value(marker.keyField).toString();
        } else if (marker.nodeIndex >= 0) {
            const QJsonArray nodes = m_root.value("keyMapNodes").toArray();
            if (marker.nodeIndex < nodes.size()) {
                currentKey = nodes.at(marker.nodeIndex).toObject().value(marker.keyField).toString();
            }
        }
        bool keyAlreadyUsed = false;
        if (key != currentKey) {
            keyAlreadyUsed = keyIsUsed(key, marker.target == PositionTarget::SmallEyesPosition
                                                ? -1 : marker.nodeIndex);
            if (!keyAlreadyUsed && marker.target == PositionTarget::SteeringDirection
                && marker.nodeIndex >= 0) {
                const QJsonArray nodes = m_root.value("keyMapNodes").toArray();
                const QJsonObject node = nodes.at(marker.nodeIndex).toObject();
                for (const QString &directionField : QStringList { "leftKey", "rightKey", "upKey", "downKey" }) {
                    if (directionField != marker.keyField && node.value(directionField).toString() == key) {
                        keyAlreadyUsed = true;
                        break;
                    }
                }
            }
        }
        if (keyAlreadyUsed) {
            QMessageBox::information(this, tr("Key already mapped"), tr("That key is already used in this keymap."));
            return;
        }
        setMarkerKey(markerIndex, key);
    } else if (chosen == removeMapping) {
        deleteMarker(markerIndex);
    }
}

bool KeyOverlayWidget::deleteMarker(int markerIndex)
{
    if (markerIndex < 0 || markerIndex >= m_markers.size()) {
        return false;
    }
    const Marker marker = m_markers.at(markerIndex);
    if ((marker.target != PositionTarget::NodeField && marker.target != PositionTarget::MultiClickField)
        || marker.nodeIndex < 0) {
        return false;
    }

    QJsonArray nodes = m_root.value("keyMapNodes").toArray();
    if (marker.nodeIndex >= nodes.size() || !nodes.at(marker.nodeIndex).isObject()) {
        return false;
    }
    if (marker.target == PositionTarget::MultiClickField) {
        QJsonObject node = nodes.at(marker.nodeIndex).toObject();
        QJsonArray clickNodes = node.value("clickNodes").toArray();
        if (marker.childIndex < 0 || marker.childIndex >= clickNodes.size()) {
            return false;
        }
        clickNodes.removeAt(marker.childIndex);
        if (clickNodes.isEmpty()) {
            nodes.removeAt(marker.nodeIndex);
        } else {
            node.insert("clickNodes", clickNodes);
            nodes.replace(marker.nodeIndex, node);
        }
    } else {
        nodes.removeAt(marker.nodeIndex);
    }
    m_root.insert("keyMapNodes", nodes);
    rebuildMarkers();
    return true;
}

QString KeyOverlayWidget::serializedScript() const
{
    return QString::fromUtf8(QJsonDocument(m_root).toJson(QJsonDocument::Indented));
}

void KeyOverlayWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_editMode) {
        event->ignore();
        return;
    }
    setFocus(Qt::MouseFocusReason);
    if (event->button() == Qt::LeftButton) {
        if (!m_editToolbar->geometry().contains(event->pos()) && !m_pendingKey.isEmpty()) {
            if (m_selectedMarkerIndex >= 0 && m_selectedMarkerIndex < m_markers.size()) {
                m_draggingMarker = m_selectedMarkerIndex;
                m_draggingEndPoint = false;
                setCursor(Qt::ClosedHandCursor);
            } else {
                m_placingPendingKey = true;
                setCursor(Qt::ClosedHandCursor);
            }
            event->accept();
            return;
        }
        bool atEnd = false;
        const int markerIndex = markerAt(event->pos(), &atEnd);
        if (markerIndex >= 0) {
            if (m_markers.at(markerIndex).target != PositionTarget::ReadOnly) {
                m_draggingMarker = markerIndex;
                m_draggingEndPoint = atEnd;
                setCursor(Qt::ClosedHandCursor);
            }
        } else if (!m_editToolbar->geometry().contains(event->pos())) {
            addMappingAt(event->pos());
        }
        event->accept();
        return;
    }
    if (event->button() == Qt::RightButton) {
        bool atEnd = false;
        const int markerIndex = markerAt(event->pos(), &atEnd);
        Q_UNUSED(atEnd)
        if (markerIndex >= 0) {
            editMarker(markerIndex, event->globalPos());
        }
        event->accept();
        return;
    }
    event->accept();
}

void KeyOverlayWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_editMode) {
        event->ignore();
        return;
    }
    if (m_draggingMarker >= 0 && (event->buttons() & Qt::LeftButton)) {
        setMarkerPosition(m_draggingMarker, m_draggingEndPoint, event->pos());
        event->accept();
        return;
    }
    if (!m_pendingKey.isEmpty()) {
        update();
    }
    bool atEnd = false;
    const int markerIndex = markerAt(event->pos(), &atEnd);
    Q_UNUSED(atEnd)
    if (markerIndex >= 0 && m_markers.at(markerIndex).target != PositionTarget::ReadOnly) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }
    event->accept();
}

void KeyOverlayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_editMode) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        if (m_placingPendingKey) {
            m_placingPendingKey = false;
            addMappingAt(event->pos());
            setCursor(Qt::CrossCursor);
            event->accept();
            return;
        }
        if (m_selectedMarkerIndex >= 0 && m_draggingMarker == m_selectedMarkerIndex) {
            setMarkerPosition(m_draggingMarker, m_draggingEndPoint, event->pos());
            m_draggingMarker = -1;
            m_selectedMarkerIndex = -1;
            m_pendingKey.clear();
            m_editHint->setText(tr("Press a key • drag or click to place"));
            setCursor(Qt::CrossCursor);
            update();
            event->accept();
            return;
        }
        m_draggingMarker = -1;
        setCursor(Qt::OpenHandCursor);
    }
    event->accept();
}

void KeyOverlayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal keyScale = qBound(0.5, m_keySize, 2.0);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(qMax(6, qRound(qBound(11, width() / 55, 18) * keyScale)));
    painter.setFont(font);
    const QFontMetrics metrics(font);

    const QRectF bounds = rect();
    const qreal badgeHeight = metrics.height() + 8.0 * keyScale;
    const qreal horizontalPadding = 16.0 * keyScale;
    painter.save();
    painter.setOpacity(qBound(0.2, m_keyOpacity, 1.0));
    for (int markerIndex = 0; markerIndex < m_markers.size(); ++markerIndex) {
        const Marker &marker = m_markers.at(markerIndex);
        if (marker.position.x() < 0.0 || marker.position.x() > 1.0
            || marker.position.y() < 0.0 || marker.position.y() > 1.0) {
            continue;
        }

        const QPointF point(marker.position.x() * width(), marker.position.y() * height());
        if (marker.hasEndPosition
            && marker.endPosition.x() >= 0.0 && marker.endPosition.x() <= 1.0
            && marker.endPosition.y() >= 0.0 && marker.endPosition.y() <= 1.0) {
            const QPointF endPoint(marker.endPosition.x() * width(), marker.endPosition.y() * height());
            painter.setPen(QPen(QColor(72, 205, 255, 205), 2.0 * keyScale, Qt::DashLine));
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
                const qreal arrowLength = 7.0 * keyScale;
                const QPointF arrowLeft(arrowTip.x() - arrowLength * std::cos(angle - 0.55),
                                        arrowTip.y() - arrowLength * std::sin(angle - 0.55));
                const QPointF arrowRight(arrowTip.x() - arrowLength * std::cos(angle + 0.55),
                                         arrowTip.y() - arrowLength * std::sin(angle + 0.55));
                painter.drawLine(arrowTip, arrowLeft);
                painter.drawLine(arrowTip, arrowRight);
            }
            painter.setPen(QPen(QColor(72, 205, 255, 235), 1.5 * keyScale));
            painter.setBrush(QColor(7, 15, 25, 205));
            const qreal endpointRadius = qMax(4.0 * keyScale, badgeHeight / 3.2);
            painter.drawEllipse(endPoint, endpointRadius, endpointRadius);
        }

        const qreal badgeWidth = qMax(badgeHeight + 2.0 * keyScale,
                                      metrics.horizontalAdvance(marker.label) + horizontalPadding);
        QRectF badge(point.x() - badgeWidth / 2.0, point.y() - badgeHeight / 2.0, badgeWidth, badgeHeight);
        badge.moveLeft(qBound(bounds.left(), badge.left(), bounds.right() - badge.width()));
        badge.moveTop(qBound(bounds.top(), badge.top(), bounds.bottom() - badge.height()));

        const QColor markerColor = markerIndex == m_selectedMarkerIndex
            ? QColor(255, 190, 64, 255) : QColor(72, 205, 255, 235);
        if (markerIndex == m_selectedMarkerIndex) {
            painter.setPen(QPen(markerColor, 2.5 * keyScale));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(badge.adjusted(-4.0 * keyScale, -4.0 * keyScale,
                                                    4.0 * keyScale, 4.0 * keyScale),
                                    badgeHeight / 2.0 + 3.0 * keyScale,
                                    badgeHeight / 2.0 + 3.0 * keyScale);
        }
        painter.setPen(QPen(markerColor, 1.5 * keyScale));
        painter.setBrush(QColor(7, 15, 25, 205));
        painter.drawRoundedRect(badge, badgeHeight / 2.0, badgeHeight / 2.0);
        painter.setPen(Qt::white);
        painter.drawText(badge, Qt::AlignCenter, marker.label);
    }
    painter.restore();

    if (m_editMode) {
        painter.setPen(QPen(QColor(72, 205, 255, 175), 1.0, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(1, 1, -2, -2));
    }

    if (m_editMode && !m_pendingKey.isEmpty()) {
        const QPoint cursor = mapFromGlobal(QCursor::pos());
        const QString label = displayKey(m_pendingKey);
        const qreal previewWidth = metrics.horizontalAdvance(label) + 18.0 * keyScale;
        QRectF preview(cursor.x() + 14.0, cursor.y() + 14.0, previewWidth, badgeHeight);
        preview.moveLeft(qBound(bounds.left(), preview.left(), bounds.right() - preview.width()));
        preview.moveTop(qBound(bounds.top(), preview.top(), bounds.bottom() - preview.height()));
        painter.setPen(QPen(m_selectedMarkerIndex >= 0
                                ? QColor(255, 190, 64, 255) : QColor(72, 205, 255, 255), 1.5));
        painter.setBrush(QColor(7, 15, 25, 220));
        painter.drawRoundedRect(preview, badgeHeight / 2.0, badgeHeight / 2.0);
        painter.setPen(Qt::white);
        painter.drawText(preview, Qt::AlignCenter, label);
    }
}
