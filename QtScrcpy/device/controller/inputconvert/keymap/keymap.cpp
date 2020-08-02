#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaEnum>

#include "keymap.h"

QString KeyMap::s_keyMapPath = "";

KeyMap::KeyMap(QObject *parent) : QObject(parent) {}

KeyMap::~KeyMap() {}

const QString &KeyMap::getKeyMapPath()
{
    if (s_keyMapPath.isEmpty()) {
        s_keyMapPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_KEYMAP_PATH"));
        QFileInfo fileInfo(s_keyMapPath);
        if (s_keyMapPath.isEmpty() || !fileInfo.isDir()) {
            s_keyMapPath = QCoreApplication::applicationDirPath() + "/keymap";
        }
    }
    return s_keyMapPath;
}

void KeyMap::loadKeyMap(const QString &json)
{
    QString errorString;
    QJsonParseError jsonError;
    QJsonDocument jsonDoc;
    QJsonObject rootObj;
    QPair<ActionType, int> switchKey;

    jsonDoc = QJsonDocument::fromJson(json.toUtf8(), &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        errorString = QString("json error: %1").arg(jsonError.errorString());
        goto parseError;
    }

    // switchKey
    rootObj = jsonDoc.object();

    if (!checkItemString(rootObj, "switchKey")) {
        errorString = QString("json error: no find switchKey");
        goto parseError;
    }

    switchKey = getItemKey(rootObj, "switchKey");
    if (switchKey.first == AT_INVALID) {
        errorString = QString("json error: switchKey invalid");
        goto parseError;
    }

    m_switchKey.type = switchKey.first;
    m_switchKey.key = switchKey.second;

    // mouseMoveMap
    if (checkItemObject(rootObj, "mouseMoveMap")) {
        QJsonObject mouseMoveMap = getItemObject(rootObj, "mouseMoveMap");
        KeyMapNode keyMapNode;
        keyMapNode.type = KMT_MOUSE_MOVE;

        if (!checkItemDouble(mouseMoveMap, "speedRatio")) {
            errorString = QString("json error: mouseMoveMap on find speedRatio");
            goto parseError;
        }
        keyMapNode.data.mouseMove.speedRatio = static_cast<int>(getItemDouble(mouseMoveMap, "speedRatio"));

        if (!checkItemObject(mouseMoveMap, "startPos")) {
            errorString = QString("json error: mouseMoveMap on find startPos");
            goto parseError;
        }
        QJsonObject startPos = mouseMoveMap.value("startPos").toObject();
        if (checkItemDouble(startPos, "x")) {
            keyMapNode.data.mouseMove.startPos.setX(getItemDouble(startPos, "x"));
        }
        if (checkItemDouble(startPos, "y")) {
            keyMapNode.data.mouseMove.startPos.setY(getItemDouble(startPos, "y"));
        }

        // small eyes
        if (checkItemObject(mouseMoveMap, "smallEyes")) {
            QJsonObject smallEyes = mouseMoveMap.value("smallEyes").toObject();
            if (!smallEyes.contains("type") || !smallEyes.value("type").isString()) {
                errorString = QString("json error: smallEyes no find node type");
                goto parseError;
            }

            // type just support KMT_CLICK
            KeyMap::KeyMapType type = getItemKeyMapType(smallEyes, "type");
            if (KeyMap::KMT_CLICK != type) {
                errorString = QString("json error: smallEyes just support KMT_CLICK");
                goto parseError;
            }

            // safe check
            if (!checkForClick(smallEyes)) {
                errorString = QString("json error: smallEyes node format error");
                goto parseError;
            }

            QPair<ActionType, int> key = getItemKey(smallEyes, "key");
            if (key.first == AT_INVALID) {
                errorString = QString("json error: keyMapNodes node invalid key: %1").arg(smallEyes.value("key").toString());
                goto parseError;
            }

            keyMapNode.data.mouseMove.smallEyes.type = key.first;
            keyMapNode.data.mouseMove.smallEyes.key = key.second;
            keyMapNode.data.mouseMove.smallEyes.pos = getItemPos(smallEyes, "pos");
        }

        m_idxMouseMove = m_keyMapNodes.size();
        m_keyMapNodes.push_back(keyMapNode);
    }

    // keyMapNodes
    if (rootObj.contains("keyMapNodes") && rootObj.value("keyMapNodes").isArray()) {
        QJsonArray keyMapNodes = rootObj.value("keyMapNodes").toArray();
        QJsonObject node;
        int size = keyMapNodes.size();
        for (int i = 0; i < size; i++) {
            if (!keyMapNodes.at(i).isObject()) {
                errorString = QString("json error: keyMapNodes node must be json object");
                goto parseError;
            }
            node = keyMapNodes.at(i).toObject();
            if (!node.contains("type") || !node.value("type").isString()) {
                errorString = QString("json error: keyMapNodes no find node type");
                goto parseError;
            }

            KeyMap::KeyMapType type = getItemKeyMapType(node, "type");
            switch (type) {
            case KeyMap::KMT_CLICK: {
                // safe check
                if (!checkForClick(node)) {
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }
                QPair<ActionType, int> key = getItemKey(node, "key");
                if (key.first == AT_INVALID) {
                    qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
                    break;
                }
                KeyMapNode keyMapNode;
                keyMapNode.type = type;
                keyMapNode.data.click.keyNode.type = key.first;
                keyMapNode.data.click.keyNode.key = key.second;
                keyMapNode.data.click.keyNode.pos = getItemPos(node, "pos");
                keyMapNode.data.click.switchMap = getItemBool(node, "switchMap");
                m_keyMapNodes.push_back(keyMapNode);
            } break;
            case KeyMap::KMT_CLICK_TWICE: {
                // safe check
                if (!checkForClickTwice(node)) {
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }

                QPair<ActionType, int> key = getItemKey(node, "key");
                if (key.first == AT_INVALID) {
                    qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
                    break;
                }
                KeyMapNode keyMapNode;
                keyMapNode.type = type;
                keyMapNode.data.click.keyNode.type = key.first;
                keyMapNode.data.click.keyNode.key = key.second;
                keyMapNode.data.click.keyNode.pos = getItemPos(node, "pos");
                keyMapNode.data.click.switchMap = getItemBool(node, "switchMap");
                m_keyMapNodes.push_back(keyMapNode);
            } break;
            case KeyMap::KMT_CLICK_MULTI: {
                // safe check
                if (!checkForClickMulti(node)) {
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }
                QPair<ActionType, int> key = getItemKey(node, "key");
                if (key.first == AT_INVALID) {
                    qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
                    break;
                }
                KeyMapNode keyMapNode;
                keyMapNode.type = type;
                keyMapNode.data.clickMulti.keyNode.type = key.first;
                keyMapNode.data.clickMulti.keyNode.key = key.second;

                QJsonArray clickNodes = node.value("clickNodes").toArray();
                QJsonObject clickNode;
                keyMapNode.data.clickMulti.keyNode.delayClickNodesCount = 0;

                for (int i = 0; i < clickNodes.size(); i++) {
                    if (i >= MAX_DELAY_CLICK_NODES) {
                        qInfo() << "clickNodes too much, up to " << MAX_DELAY_CLICK_NODES;
                        break;
                    }
                    clickNode = clickNodes.at(i).toObject();
                    DelayClickNode delayClickNode;
                    delayClickNode.delay = getItemDouble(clickNode, "delay");
                    delayClickNode.pos = getItemPos(clickNode, "pos");
                    keyMapNode.data.clickMulti.keyNode.delayClickNodes[i] = delayClickNode;
                    keyMapNode.data.clickMulti.keyNode.delayClickNodesCount++;
                }

                m_keyMapNodes.push_back(keyMapNode);
            } break;
            case KeyMap::KMT_STEER_WHEEL: {
                // safe check
                if (!checkForSteerWhell(node)) {
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }
                QPair<ActionType, int> leftKey = getItemKey(node, "leftKey");
                QPair<ActionType, int> rightKey = getItemKey(node, "rightKey");
                QPair<ActionType, int> upKey = getItemKey(node, "upKey");
                QPair<ActionType, int> downKey = getItemKey(node, "downKey");
                if (leftKey.first == AT_INVALID || rightKey.first == AT_INVALID || upKey.first == AT_INVALID || downKey.first == AT_INVALID) {
                    if (leftKey.first == AT_INVALID) {
                        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("leftKey").toString();
                    }
                    if (rightKey.first == AT_INVALID) {
                        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("rightKey").toString();
                    }
                    if (upKey.first == AT_INVALID) {
                        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("upKey").toString();
                    }
                    if (downKey.first == AT_INVALID) {
                        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("downKey").toString();
                    }
                    break;
                }

                KeyMapNode keyMapNode;
                keyMapNode.type = type;

                keyMapNode.data.steerWheel.left = { leftKey.first, leftKey.second, QPointF(0, 0), QPointF(0, 0), getItemDouble(node, "leftOffset") };
                keyMapNode.data.steerWheel.right = { rightKey.first, rightKey.second, QPointF(0, 0), QPointF(0, 0), getItemDouble(node, "rightOffset") };
                keyMapNode.data.steerWheel.up = { upKey.first, upKey.second, QPointF(0, 0), QPointF(0, 0), getItemDouble(node, "upOffset") };
                keyMapNode.data.steerWheel.down = { downKey.first, downKey.second, QPointF(0, 0), QPointF(0, 0), getItemDouble(node, "downOffset") };

                keyMapNode.data.steerWheel.centerPos = getItemPos(node, "centerPos");
                m_idxSteerWheel = m_keyMapNodes.size();
                m_keyMapNodes.push_back(keyMapNode);
            } break;
            case KeyMap::KMT_DRAG: {
                // safe check
                if (!checkForDrag(node)) {
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }

                QPair<ActionType, int> key = getItemKey(node, "key");
                if (key.first == AT_INVALID) {
                    qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
                    break;
                }
                KeyMapNode keyMapNode;
                keyMapNode.type = type;
                keyMapNode.data.drag.keyNode.type = key.first;
                keyMapNode.data.drag.keyNode.key = key.second;
                keyMapNode.data.drag.keyNode.pos = getItemPos(node, "startPos");
                keyMapNode.data.drag.keyNode.extendPos = getItemPos(node, "endPos");
                m_keyMapNodes.push_back(keyMapNode);
                break;
            }
            default:
                qWarning() << "json error: keyMapNodes invalid node type:" << node.value("type").toString();
                break;
            }
        }
    }
    // this must be called after m_keyMapNodes is stable
    makeReverseMap();
    qInfo() << tr("Script updated, current keymap mode:normal, Press ~ key to switch keymap mode");

parseError:
    if (!errorString.isEmpty()) {
        qWarning() << errorString;
    }
    return;
}

const KeyMap::KeyMapNode &KeyMap::getKeyMapNode(int key)
{
    auto p = m_rmapKey.value(key, &m_invalidNode);
    if (p == &m_invalidNode) {
        return *m_rmapMouse.value(key, &m_invalidNode);
    }
    return *p;
}

const KeyMap::KeyMapNode &KeyMap::getKeyMapNodeKey(int key)
{
    return *m_rmapKey.value(key, &m_invalidNode);
}

const KeyMap::KeyMapNode &KeyMap::getKeyMapNodeMouse(int key)
{
    return *m_rmapMouse.value(key, &m_invalidNode);
}

bool KeyMap::isSwitchOnKeyboard()
{
    return m_switchKey.type == AT_KEY;
}

int KeyMap::getSwitchKey()
{
    return m_switchKey.key;
}

const KeyMap::KeyMapNode &KeyMap::getMouseMoveMap()
{
    return m_keyMapNodes[m_idxMouseMove];
}

bool KeyMap::isValidMouseMoveMap()
{
    return m_idxMouseMove != -1;
}

bool KeyMap::isValidSteerWheelMap()
{
    return m_idxSteerWheel != -1;
}

void KeyMap::makeReverseMap()
{
    m_rmapKey.clear();
    m_rmapMouse.clear();
    for (int i = 0; i < m_keyMapNodes.size(); ++i) {
        auto &node = m_keyMapNodes[i];
        switch (node.type) {
        case KMT_CLICK: {
            QMultiHash<int, KeyMapNode *> &m = node.data.click.keyNode.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            m.insert(node.data.click.keyNode.key, &node);
        } break;
        case KMT_CLICK_TWICE: {
            QMultiHash<int, KeyMapNode *> &m = node.data.clickTwice.keyNode.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            m.insert(node.data.clickTwice.keyNode.key, &node);
        } break;
        case KMT_CLICK_MULTI: {
            QMultiHash<int, KeyMapNode *> &m = node.data.clickMulti.keyNode.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            m.insert(node.data.clickMulti.keyNode.key, &node);
        } break;
        case KMT_STEER_WHEEL: {
            QMultiHash<int, KeyMapNode *> &ml = node.data.steerWheel.left.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            ml.insert(node.data.steerWheel.left.key, &node);
            QMultiHash<int, KeyMapNode *> &mr = node.data.steerWheel.right.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            mr.insert(node.data.steerWheel.right.key, &node);
            QMultiHash<int, KeyMapNode *> &mu = node.data.steerWheel.up.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            mu.insert(node.data.steerWheel.up.key, &node);
            QMultiHash<int, KeyMapNode *> &md = node.data.steerWheel.down.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            md.insert(node.data.steerWheel.down.key, &node);
        } break;
        case KMT_DRAG: {
            QMultiHash<int, KeyMapNode *> &m = node.data.drag.keyNode.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            m.insert(node.data.drag.keyNode.key, &node);
        } break;
        default:
            break;
        }
    }
}

QString KeyMap::getItemString(const QJsonObject &node, const QString &name)
{
    return node.value(name).toString();
}

double KeyMap::getItemDouble(const QJsonObject &node, const QString &name)
{
    return node.value(name).toDouble();
}

bool KeyMap::getItemBool(const QJsonObject &node, const QString &name)
{
    return node.value(name).toBool(false);
}

QJsonObject KeyMap::getItemObject(const QJsonObject &node, const QString &name)
{
    return node.value(name).toObject();
}

QPointF KeyMap::getItemPos(const QJsonObject &node, const QString &name)
{
    QJsonObject pos = node.value(name).toObject();
    return QPointF(pos.value("x").toDouble(), pos.value("y").toDouble());
}

QPair<KeyMap::ActionType, int> KeyMap::getItemKey(const QJsonObject &node, const QString &name)
{
    QString value = getItemString(node, name);
    int key = m_metaEnumKey.keyToValue(value.toStdString().c_str());
    int btn = m_metaEnumMouseButtons.keyToValue(value.toStdString().c_str());
    if (key == -1 && btn == -1) {
        return { AT_INVALID, -1 };
    } else if (key != -1) {
        return { AT_KEY, key };
    } else {
        return { AT_MOUSE, btn };
    }
}

KeyMap::KeyMapType KeyMap::getItemKeyMapType(const QJsonObject &node, const QString &name)
{
    QString value = getItemString(node, name);
    return static_cast<KeyMap::KeyMapType>(m_metaEnumKeyMapType.keyToValue(value.toStdString().c_str()));
}

bool KeyMap::checkItemString(const QJsonObject &node, const QString &name)
{
    return node.contains(name) && node.value(name).isString();
}

bool KeyMap::checkItemDouble(const QJsonObject &node, const QString &name)
{
    return node.contains(name) && node.value(name).isDouble();
}

bool KeyMap::checkItemBool(const QJsonObject &node, const QString &name)
{
    return node.contains(name) && node.value(name).isBool();
}

bool KeyMap::checkItemObject(const QJsonObject &node, const QString &name)
{
    return node.contains(name) && node.value(name).isObject();
}

bool KeyMap::checkItemPos(const QJsonObject &node, const QString &name)
{
    if (node.contains(name) && node.value(name).isObject()) {
        QJsonObject pos = node.value(name).toObject();
        return pos.contains("x") && pos.value("x").isDouble() && pos.contains("y") && pos.value("y").isDouble();
    }
    return false;
}

bool KeyMap::checkForClick(const QJsonObject &node)
{
    return checkForClickTwice(node) && checkItemBool(node, "switchMap");
}

bool KeyMap::checkForClickMulti(const QJsonObject &node)
{
    bool ret = true;

    if (!node.contains("clickNodes") || !node.value("clickNodes").isArray()) {
        qWarning("json error: no find clickNodes");
        return false;
    }

    QJsonArray clickNodes = node.value("clickNodes").toArray();
    QJsonObject clickNode;
    int size = clickNodes.size();
    if (0 == size) {
        qWarning("json error: clickNodes is empty");
        return false;
    }

    for (int i = 0; i < size; i++) {
        if (!clickNodes.at(i).isObject()) {
            qWarning("json error: clickNodes node must be json object");
            ret = false;
            break;
        }

        clickNode = clickNodes.at(i).toObject();
        if (!checkForDelayClickNode(clickNode)) {
            ret = false;
            break;
        }
    }

    return ret;
}

bool KeyMap::checkForDelayClickNode(const QJsonObject &node)
{
    return checkItemPos(node, "pos") && checkItemDouble(node, "delay");
}

bool KeyMap::checkForClickTwice(const QJsonObject &node)
{
    return checkItemString(node, "key") && checkItemPos(node, "pos");
}

bool KeyMap::checkForSteerWhell(const QJsonObject &node)
{
    return checkItemString(node, "leftKey") && checkItemString(node, "rightKey") && checkItemString(node, "upKey") && checkItemString(node, "downKey")
           && checkItemDouble(node, "leftOffset") && checkItemDouble(node, "rightOffset") && checkItemDouble(node, "upOffset")
           && checkItemDouble(node, "downOffset") && checkItemPos(node, "centerPos");
}

bool KeyMap::checkForDrag(const QJsonObject &node)
{
    return checkItemString(node, "key") && checkItemPos(node, "startPos") && checkItemPos(node, "endPos");
}
