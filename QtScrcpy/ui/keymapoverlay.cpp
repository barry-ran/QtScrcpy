#include "keymapoverlay.h"
#include "keymapnodewidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QInputDialog>
#include <QJsonParseError>
#include <QMetaEnum>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSaveFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>

#include "../QtScrcpyCore/src/device/controller/inputconvert/keymap/keymap.h"

static bool isSupportedKeyName(const QString &key)
{
    const QByteArray keyName = key.trimmed().toLatin1();
    return QMetaEnum::fromType<Qt::Key>().keyToValue(keyName.constData()) != -1
        || QMetaEnum::fromType<Qt::MouseButtons>().keyToValue(keyName.constData()) != -1;
}

static bool hasEditableClickSequence(const QJsonObject &action)
{
    const QJsonValue sequenceValue = action.value(QStringLiteral("clickNodes"));
    if (!sequenceValue.isArray()) {
        return false;
    }
    const QJsonArray sequence = sequenceValue.toArray();
    if (sequence.isEmpty() || sequence.size() > 50) {
        return false;
    }
    for (const QJsonValue &value : sequence) {
        if (!value.isObject()) {
            return false;
        }
        const QJsonObject step = value.toObject();
        const QJsonValue positionValue = step.value(QStringLiteral("pos"));
        const QJsonValue delayValue = step.value(QStringLiteral("delay"));
        if (!positionValue.isObject() || !delayValue.isDouble()
            || delayValue.toDouble() < 0.0 || delayValue.toDouble() > 2147483647.0) {
            return false;
        }
        const QJsonObject position = positionValue.toObject();
        if (!position.value(QStringLiteral("x")).isDouble()
            || !position.value(QStringLiteral("y")).isDouble()) {
            return false;
        }
    }
    return true;
}

KeymapOverlay::KeymapOverlay(QWidget *parent)
    : QWidget(parent), m_editMode(false)
{
    // Transparent, stays on top of video widget
    setAttribute(Qt::WA_TranslucentBackground);
    // Don't consume mouse events when not editing
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    auto *panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("keymapEditorPanel"));
    panel->setFrameShape(QFrame::StyledPanel);
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(10, 10, 10, 10);
    panelLayout->setSpacing(8);

    auto *header = new QHBoxLayout();
    auto *title = new QLabel(tr("Key Mapping"), panel);
    title->setObjectName(QStringLiteral("keymapEditorTitle"));
    auto *closeButton = new QPushButton(tr("×"), panel);
    closeButton->setObjectName(QStringLiteral("keymapEditorClose"));
    closeButton->setFixedSize(28, 28);
    closeButton->setToolTip(tr("Exit edit mode; unsaved changes remain in the editor"));
    header->addWidget(title, 1);
    header->addWidget(closeButton);
    panelLayout->addLayout(header);

    auto *hint = new QLabel(tr("Choose an action, then click the game view to place it. Drag a key to move it; double-click to configure."), panel);
    hint->setObjectName(QStringLiteral("keymapEditorHint"));
    hint->setWordWrap(true);
    panelLayout->addWidget(hint);

    auto *scrollArea = new QScrollArea(panel);
    scrollArea->setObjectName(QStringLiteral("keymapActionScroll"));
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    m_toolPanelBody = new QWidget(scrollArea);
    auto *actionsLayout = new QGridLayout(m_toolPanelBody);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(6);

    struct ActionChoice {
        const char *label;
        const char *action;
    };
    const ActionChoice actions[] = {
        {QT_TR_NOOP("Direction"), "direction"},
        {QT_TR_NOOP("Fire button"), "fire"},
        {QT_TR_NOOP("Aim / View"), "camera"},
        {QT_TR_NOOP("Slide / Drag"), "drag"},
        {QT_TR_NOOP("Multi-tap"), "multiTap"},
        {QT_TR_NOOP("Double tap"), "doubleTap"},
        {QT_TR_NOOP("Custom key"), "custom"}
    };
    for (int i = 0; i < static_cast<int>(sizeof(actions) / sizeof(actions[0])); ++i) {
        auto *button = new QPushButton(tr(actions[i].label), m_toolPanelBody);
        button->setObjectName(QStringLiteral("keymapActionButton"));
        button->setMinimumHeight(38);
        const QString action = QString::fromLatin1(actions[i].action);
        connect(button, &QPushButton::clicked, this, [this, action]() {
            beginAddAction(action);
        });
        actionsLayout->addWidget(button, i / 2, i % 2);
    }
    scrollArea->setWidget(m_toolPanelBody);
    panelLayout->addWidget(scrollArea, 1);

    m_panelStatus = new QLabel(tr("Select an action to place it."), panel);
    m_panelStatus->setObjectName(QStringLiteral("keymapEditorStatus"));
    m_panelStatus->setWordWrap(true);
    panelLayout->addWidget(m_panelStatus);

    m_finishMultiTapButton = new QPushButton(tr("Finish multi-tap"), panel);
    m_finishMultiTapButton->setObjectName(QStringLiteral("keymapEditorFinish"));
    m_finishMultiTapButton->setVisible(false);
    panelLayout->addWidget(m_finishMultiTapButton);

    auto *footer = new QHBoxLayout();
    auto *clearButton = new QPushButton(tr("Clear keys"), panel);
    clearButton->setObjectName(QStringLiteral("keymapEditorClear"));
    auto *saveButton = new QPushButton(tr("Save"), panel);
    saveButton->setObjectName(QStringLiteral("keymapEditorSave"));
    footer->addWidget(clearButton);
    footer->addWidget(saveButton, 1);
    panelLayout->addLayout(footer);

    m_toolPanel = panel;
    connect(closeButton, &QPushButton::clicked, this, &KeymapOverlay::closeRequested);
    connect(clearButton, &QPushButton::clicked, this, &KeymapOverlay::clearKeymap);
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        finishMultiTap();
        emit saveRequested();
    });
    connect(m_finishMultiTapButton, &QPushButton::clicked, this, &KeymapOverlay::finishMultiTap);
    m_toolPanel->hide();
}

KeymapOverlay::~KeymapOverlay()
{
    qDeleteAll(m_nodes);
    m_nodes.clear();
}

void KeymapOverlay::setEditMode(bool edit)
{
    m_editMode = edit;
    setAttribute(Qt::WA_TransparentForMouseEvents, !m_editMode);
    m_pendingAction.clear();
    m_pendingDragNode = nullptr;
    if (m_pendingMultiClickNode) {
        m_pendingMultiClickNode->setPlacementMode(false);
    }
    m_pendingMultiClickNode = nullptr;
    if (m_finishMultiTapButton) {
        m_finishMultiTapButton->hide();
    }
    if (m_panelStatus) {
        m_panelStatus->setText(tr("Select an action to place it."));
    }
    if (m_toolPanel) {
        m_toolPanel->setVisible(m_editMode);
        if (m_editMode) {
            updatePanelGeometry();
        }
    }

    // Show or hide all nodes
    for (auto *node : m_nodes) {
        node->setVisible(m_editMode);
    }
    if (m_editMode && m_toolPanel) {
        m_toolPanel->raise();
    }

    update();
}

void KeymapOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (!m_editMode) {
        return;
    }

    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 70));
    for (const auto *node : m_nodes) {
        if (const auto *dragNode = qobject_cast<const DragNodeWidget *>(node)) {
            dragNode->paintGuide(painter);
        } else if (const auto *multiClickNode = qobject_cast<const MultiClickNodeWidget *>(node)) {
            multiClickNode->paintGuide(painter);
        }
    }

    // Green border
    painter.setPen(QPen(QColor(0, 230, 0), 2));
    painter.drawRect(rect().adjusted(1, 1, -1, -1));

    // Instructions
    painter.setPen(QColor(255, 255, 255, 210));
    QFont f = painter.font();
    f.setPointSize(9);
    painter.setFont(f);
    painter.drawText(rect().adjusted(8, 8, -8, -8),
                     Qt::AlignTop | Qt::AlignLeft,
                     tr("Choose an action in the palette  |  Drag to move  |  Double-click a key to edit  |  [X] to remove"));
}

void KeymapOverlay::resizeEvent(QResizeEvent *event)
{
    const QSize oldSize = event->oldSize();
    const QSize newSize = event->size();
    if (!oldSize.isEmpty() && !newSize.isEmpty()) {
        for (auto *node : m_nodes) {
            const QPoint oldCenter = node->geometry().center();
            const double xRatio = static_cast<double>(oldCenter.x()) / oldSize.width();
            const double yRatio = static_cast<double>(oldCenter.y()) / oldSize.height();
            const int x = qRound(xRatio * newSize.width() - node->width() / 2.0);
            const int y = qRound(yRatio * newSize.height() - node->height() / 2.0);
            node->move(qBound(0, x, qMax(0, newSize.width() - node->width())),
                       qBound(0, y, qMax(0, newSize.height() - node->height())));
        }
    }
    updatePanelGeometry();
    QWidget::resizeEvent(event);
}

void KeymapOverlay::updatePanelGeometry()
{
    if (!m_toolPanel) {
        return;
    }
    const int panelWidth = qBound(150, width() / 4, 230);
    const int panelHeight = qMax(0, qMin(520, height() - 20));
    m_toolPanel->setGeometry(qMax(0, width() - panelWidth - 10), 10,
                             qMin(panelWidth, width()), qMin(panelHeight, height() - 20));
}

void KeymapOverlay::addNodeAt(KeymapNodeWidget *node, const QPoint &pos)
{
    const QPoint topLeft = pos - QPoint(node->width() / 2, node->height() / 2);
    node->move(qBound(0, topLeft.x(), qMax(0, width() - node->width())),
               qBound(0, topLeft.y(), qMax(0, height() - node->height())));
    node->show();
    registerNode(node);
    if (m_toolPanel) {
        m_toolPanel->raise();
    }
}

void KeymapOverlay::beginAddAction(const QString &action)
{
    if (m_pendingMultiClickNode) {
        finishMultiTap();
    }
    for (auto *node : m_nodes) {
        node->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    }
    if (action == QStringLiteral("camera")) {
        for (auto *existingNode : m_nodes) {
            if (qobject_cast<MouseMoveNodeWidget *>(existingNode)) {
                m_panelStatus->setText(tr("A view-angle control already exists."));
                return;
            }
        }
    }

    m_pendingAction = action;
    if (action == QStringLiteral("multiTap")) {
        m_pendingMultiClickNode = nullptr;
        for (auto *node : m_nodes) {
            node->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        }
        m_finishMultiTapButton->show();
        m_panelStatus->setText(tr("Click each touch location, then finish the sequence."));
        return;
    }
    m_finishMultiTapButton->hide();
    m_panelStatus->setText(tr("Click the game view where you want to place this control."));
}

void KeymapOverlay::finishMultiTap()
{
    if (!m_pendingMultiClickNode) {
        m_pendingAction.clear();
        m_finishMultiTapButton->hide();
        return;
    }

    const int pointCount = m_pendingMultiClickNode->clickPointCount();
    for (auto *node : m_nodes) {
        node->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    }
    m_pendingMultiClickNode = nullptr;
    m_pendingAction.clear();
    m_finishMultiTapButton->hide();
    m_panelStatus->setText(tr("Multi-tap sequence finished (%1 locations). Choose another action or save.")
                               .arg(pointCount));
}

void KeymapOverlay::clearKeymap()
{
    const auto answer = QMessageBox::question(
        this, tr("Clear keymap"), tr("Remove all key controls from this keymap?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    qDeleteAll(m_nodes);
    m_nodes.clear();
    m_nodeEntries.clear();
    m_loadedMouseMoveMap = false;
    m_document.remove(QStringLiteral("mouseMoveMap"));
    m_pendingAction.clear();
    m_pendingDragNode = nullptr;
    m_pendingMultiClickNode = nullptr;
    m_finishMultiTapButton->hide();
    m_panelStatus->setText(tr("Keymap cleared. Choose an action to add controls."));
    update();
}

void KeymapOverlay::registerNode(KeymapNodeWidget *node, const QJsonValue &preservedValue)
{
    m_nodes.append(node);
    if (!qobject_cast<MouseMoveNodeWidget *>(node)) {
        m_nodeEntries.append(qMakePair(node, preservedValue));
    }
    connect(node, &KeymapNodeWidget::interactionFinished, this, [this]() {
        if (m_toolPanel && m_editMode) {
            m_toolPanel->raise();
        }
    });
    connect(node, &KeymapNodeWidget::removeRequested, this, [this, node]() {
        if (m_pendingDragNode == node) {
            m_pendingDragNode = nullptr;
            m_pendingAction.clear();
        }
        if (m_pendingMultiClickNode == node) {
            m_pendingMultiClickNode = nullptr;
            m_pendingAction.clear();
            m_finishMultiTapButton->hide();
        }
        m_nodes.removeAll(node);
        for (int i = m_nodeEntries.size() - 1; i >= 0; --i) {
            if (m_nodeEntries.at(i).first == node) {
                m_nodeEntries.removeAt(i);
            }
        }
        node->deleteLater();
    });
}

void KeymapOverlay::mousePressEvent(QMouseEvent *event)
{
    if (m_editMode && event->button() == Qt::LeftButton
        && m_pendingAction == QStringLiteral("multiTap")) {
        if (!m_pendingMultiClickNode) {
            auto *node = new MultiClickNodeWidget(this);
            addNodeAt(node, event->pos());
            node->setPlacementMode(true);
            m_pendingMultiClickNode = node;
        }
        if (!m_pendingMultiClickNode->addClickPoint(event->pos())) {
            m_panelStatus->setText(tr("A multi-tap map can contain at most 50 locations."));
        } else {
            m_panelStatus->setText(tr("Added location %1. Click more locations or finish the sequence.")
                                       .arg(m_pendingMultiClickNode->clickPointCount()));
        }
        event->accept();
        return;
    }

    if (m_editMode && event->button() == Qt::LeftButton
        && m_pendingAction == QStringLiteral("dragEnd") && m_pendingDragNode) {
        m_pendingDragNode->setEndPoint(event->pos());
        m_pendingDragNode = nullptr;
        m_pendingAction.clear();
        m_panelStatus->setText(tr("Slide control added. Choose another action or save."));
        event->accept();
        return;
    }

    if (m_editMode && event->button() == Qt::LeftButton && !m_pendingAction.isEmpty()) {
        KeymapNodeWidget *node = nullptr;
        if (m_pendingAction == QStringLiteral("direction")) {
            node = new SteerWheelNodeWidget(this);
        } else if (m_pendingAction == QStringLiteral("camera")) {
            node = new MouseMoveNodeWidget(this);
        } else if (m_pendingAction == QStringLiteral("drag")) {
            auto *dragNode = new DragNodeWidget(this);
            addNodeAt(dragNode, event->pos());
            m_pendingDragNode = dragNode;
            m_pendingAction = QStringLiteral("dragEnd");
            m_panelStatus->setText(tr("Click the drag destination to finish the slide control."));
            event->accept();
            return;
        } else if (m_pendingAction == QStringLiteral("multiTap")) {
            event->accept();
            return;
        } else {
            auto *clickNode = new ClickNodeWidget(this);
            if (m_pendingAction == QStringLiteral("fire")) {
                clickNode->setKeyName(QStringLiteral("LeftButton"));
            } else if (m_pendingAction == QStringLiteral("doubleTap")) {
                clickNode->setActionType(QStringLiteral("KMT_CLICK_TWICE"));
                clickNode->setKeyName(QStringLiteral("Key_Q"));
            } else if (m_pendingAction == QStringLiteral("custom")) {
                bool ok = false;
                const QString key = QInputDialog::getText(
                    this, tr("Custom Key"), tr("Qt key name (for example Key_F or Key_Space):"),
                    QLineEdit::Normal, QStringLiteral("Key_F"), &ok);
                if (!ok || key.trimmed().isEmpty()) {
                    delete clickNode;
                    m_pendingAction.clear();
                    m_panelStatus->setText(tr("Action cancelled."));
                    event->accept();
                    return;
                }
                if (!isSupportedKeyName(key)) {
                    QMessageBox::warning(this, tr("Custom Key"),
                                         tr("Enter a valid Qt key or mouse-button name."));
                    delete clickNode;
                    event->accept();
                    return;
                }
                clickNode->setKeyName(key.trimmed());
            }
            node = clickNode;
        }

        addNodeAt(node, event->pos());
        m_pendingAction.clear();
        m_panelStatus->setText(tr("Control added. Choose another action or save."));
        event->accept();
        return;
    }

    if (m_editMode && event->button() == Qt::RightButton) {
        QMenu menu(this);

        // Preset options
        QAction *addFire  = menu.addAction(tr("🔥  Fire Button  (Left Click)"));
        QAction *addScope = menu.addAction(tr("🎯  Scope Button  (Right Click)"));
        menu.addSeparator();
        QAction *addWasd  = menu.addAction(tr("🕹  WASD Movement"));
        QAction *addCamera = menu.addAction(tr("👀  Camera Look (Mouse Move)"));
        menu.addSeparator();
        QAction *addCustom = menu.addAction(tr("⌨  Custom Key…"));
        QAction *addDoubleTap = menu.addAction(tr("Double Tap"));

        QAction *result = menu.exec(
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            event->globalPosition().toPoint()
#else
            event->globalPos()
#endif
        );

        if (result == addFire) {
            auto *node = new ClickNodeWidget(this);
            node->setKeyName(QStringLiteral("LeftButton"));
            addNodeAt(node, event->pos());

        } else if (result == addScope) {
            auto *node = new ClickNodeWidget(this);
            node->setKeyName(QStringLiteral("RightButton"));
            addNodeAt(node, event->pos());

        } else if (result == addWasd) {
            auto *node = new SteerWheelNodeWidget(this);
            addNodeAt(node, event->pos());

        } else if (result == addCamera) {
            // Ensure only one Camera Look node exists
            for (auto *existingNode : m_nodes) {
                if (qobject_cast<MouseMoveNodeWidget*>(existingNode)) {
                    return;
                }
            }
            auto *node = new MouseMoveNodeWidget(this);
            addNodeAt(node, event->pos());

        } else if (result == addCustom) {
            bool ok;
            QString key = QInputDialog::getText(this, tr("Custom Key"),
                tr("Qt key name (e.g.  Key_F,  Key_1,  Key_Space):"),
                QLineEdit::Normal, QStringLiteral("Key_F"), &ok);
            if (ok && !key.trimmed().isEmpty()) {
                if (!isSupportedKeyName(key)) {
                    QMessageBox::warning(this, tr("Custom Key"), tr("Enter a valid Qt key or mouse-button name."));
                } else {
                    auto *node = new ClickNodeWidget(this);
                    node->setKeyName(key.trimmed());
                    addNodeAt(node, event->pos());
                }
            }
        } else if (result == addDoubleTap) {
            bool ok;
            QString key = QInputDialog::getText(this, tr("Double Tap Key"),
                tr("Qt key name (e.g.  Key_Q,  Key_E):"),
                QLineEdit::Normal, QStringLiteral("Key_Q"), &ok);
            if (ok && !key.trimmed().isEmpty()) {
                if (!isSupportedKeyName(key)) {
                    QMessageBox::warning(this, tr("Double Tap Key"), tr("Enter a valid Qt key or mouse-button name."));
                } else {
                    auto *node = new ClickNodeWidget(this);
                    node->setActionType(QStringLiteral("KMT_CLICK_TWICE"));
                    node->setKeyName(key.trimmed());
                    addNodeAt(node, event->pos());
                }
            }
        }

        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void KeymapOverlay::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mouseDoubleClickEvent(event);
}

void KeymapOverlay::loadKeymap(const QString &jsonFilePath)
{
    QFile file(jsonFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (QFile::exists(jsonFilePath)) {
            qWarning() << "KeymapOverlay: cannot open keymap:" << jsonFilePath << file.errorString();
            m_loadFailed = true;
        }
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "KeymapOverlay: invalid JSON keymap:" << jsonFilePath
                   << parseError.errorString() << "at offset" << parseError.offset;
        m_loadFailed = true;
        return;
    }

    const QJsonObject parsedRoot = doc.object();
    if (parsedRoot.contains(QStringLiteral("keyMapNodes"))
        && !parsedRoot.value(QStringLiteral("keyMapNodes")).isArray()) {
        qWarning() << "KeymapOverlay: keyMapNodes is not an array:" << jsonFilePath;
        m_loadFailed = true;
        return;
    }
    if (parsedRoot.contains(QStringLiteral("switchKey"))
        && !parsedRoot.value(QStringLiteral("switchKey")).isString()) {
        qWarning() << "KeymapOverlay: switchKey is not a string:" << jsonFilePath;
        m_loadFailed = true;
        return;
    }
    if (parsedRoot.contains(QStringLiteral("mouseMoveMap"))
        && !parsedRoot.value(QStringLiteral("mouseMoveMap")).isObject()) {
        qWarning() << "KeymapOverlay: mouseMoveMap is not an object:" << jsonFilePath;
        m_loadFailed = true;
        return;
    }

    const QJsonArray parsedNodes = parsedRoot.value(QStringLiteral("keyMapNodes")).toArray();
    for (int i = 0; i < parsedNodes.size(); ++i) {
        if (!parsedNodes.at(i).isObject()
            || !parsedNodes.at(i).toObject().value(QStringLiteral("type")).isString()) {
            qWarning() << "KeymapOverlay: invalid keyMapNodes entry in" << jsonFilePath
                       << "at index" << i;
            m_loadFailed = true;
            return;
        }
    }

    m_loadFailed = false;
    m_document = parsedRoot;
    m_nodeEntries.clear();
    m_loadedMouseMoveMap = false;
    qDeleteAll(m_nodes);
    m_nodes.clear();

    QJsonObject root = m_document;
    QJsonArray arr = root[QStringLiteral("keyMapNodes")].toArray();

    for (const QJsonValue &val : arr) {
        QJsonObject obj = val.toObject();
        QString type = obj.value(QStringLiteral("type")).toString();

        KeymapNodeWidget *node = nullptr;
        if (type == QStringLiteral("KMT_CLICK") || type == QStringLiteral("KMT_CLICK_TWICE")
            || type == QStringLiteral("click")) {
            node = new ClickNodeWidget(this);
        } else if (type == QStringLiteral("KMT_STEER_WHEEL")) {
            node = new SteerWheelNodeWidget(this);
        } else if (type == QStringLiteral("KMT_DRAG")) {
            node = new DragNodeWidget(this);
        } else if (type == QStringLiteral("KMT_CLICK_MULTI") && hasEditableClickSequence(obj)) {
            node = new MultiClickNodeWidget(this);
        }

        if (!node) {
            m_nodeEntries.append(qMakePair(static_cast<KeymapNodeWidget *>(nullptr), val));
            continue;
        }
        node->fromJson(obj);
        node->setVisible(m_editMode);
        registerNode(node);
    }

    if (root.value(QStringLiteral("mouseMoveMap")).isObject()) {
        m_loadedMouseMoveMap = true;
        QJsonObject mouseMoveObj = root.value(QStringLiteral("mouseMoveMap")).toObject();
        auto *node = new MouseMoveNodeWidget(this);
        node->fromJson(mouseMoveObj);
        node->setVisible(m_editMode);
        registerNode(node);
    }
}

bool KeymapOverlay::saveKeymap(const QString &jsonFilePath)
{
    if (m_loadFailed) {
        qWarning() << "KeymapOverlay: refusing to overwrite an invalid keymap:" << jsonFilePath;
        return false;
    }
    const QJsonValue switchKey = m_document.value(QStringLiteral("switchKey"));
    if (!switchKey.isUndefined() && (!switchKey.isString() || !isSupportedKeyName(switchKey.toString()))) {
        qWarning() << "KeymapOverlay: refusing to save an invalid switchKey:" << jsonFilePath;
        return false;
    }
    for (const auto *node : m_nodes) {
        if (const auto *clickNode = qobject_cast<const ClickNodeWidget *>(node)) {
            if (!isSupportedKeyName(clickNode->keyName())) {
                qWarning() << "KeymapOverlay: refusing to save an invalid click key:" << clickNode->keyName();
                return false;
            }
        } else if (const auto *steerNode = qobject_cast<const SteerWheelNodeWidget *>(node)) {
            if (!isSupportedKeyName(steerNode->upKey()) || !isSupportedKeyName(steerNode->downKey())
                || !isSupportedKeyName(steerNode->leftKey()) || !isSupportedKeyName(steerNode->rightKey())) {
                qWarning() << "KeymapOverlay: refusing to save an invalid steering-wheel key";
                return false;
            }
        } else if (const auto *dragNode = qobject_cast<const DragNodeWidget *>(node)) {
            if (!isSupportedKeyName(dragNode->keyName())) {
                qWarning() << "KeymapOverlay: refusing to save an invalid slide key:" << dragNode->keyName();
                return false;
            }
        } else if (const auto *mouseMoveNode = qobject_cast<const MouseMoveNodeWidget *>(node)) {
            if (!isSupportedKeyName(mouseMoveNode->switchKey())) {
                qWarning() << "KeymapOverlay: refusing to save an invalid camera toggle key:"
                           << mouseMoveNode->switchKey();
                return false;
            }
        }
    }

    QJsonObject root = m_document;
    QJsonArray arr;
    bool hasMouseMoveMap = false;
    if (m_loadedMouseMoveMap) {
        root.remove(QStringLiteral("mouseMoveMap"));
    }
    for (const auto *node : m_nodes) {
        if (const auto *mouseMoveNode = qobject_cast<const MouseMoveNodeWidget *>(node)) {
            root[QStringLiteral("mouseMoveMap")] = mouseMoveNode->toJson();
            hasMouseMoveMap = true;
        }
    }
    if (m_loadedMouseMoveMap && !hasMouseMoveMap) {
        root.remove(QStringLiteral("mouseMoveMap"));
    }
    for (const auto &entry : m_nodeEntries) {
        arr.append(entry.first ? entry.first->toJson() : entry.second);
    }

    if (!root.contains(QStringLiteral("switchKey"))) {
        root[QStringLiteral("switchKey")] = QStringLiteral("Key_QuoteLeft");
    }
    root[QStringLiteral("keyMapNodes")] = arr;
    QJsonDocument doc(root);
    KeyMap validator;
    if (!validator.loadKeyMap(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)), false)) {
        qWarning() << "KeymapOverlay: refusing to save a keymap rejected by the runtime parser:"
                   << jsonFilePath;
        return false;
    }

    QSaveFile file(jsonFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "KeymapOverlay: cannot write keymap to:" << jsonFilePath << file.errorString();
        return false;
    }
    const QByteArray data = doc.toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        qWarning() << "KeymapOverlay: incomplete keymap write to:" << jsonFilePath << file.errorString();
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        qWarning() << "KeymapOverlay: cannot commit keymap to:" << jsonFilePath << file.errorString();
        return false;
    }
    m_document = root;
    m_loadedMouseMoveMap = hasMouseMoveMap;
    return true;
}
