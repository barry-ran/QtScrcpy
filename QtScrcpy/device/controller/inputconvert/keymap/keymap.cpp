#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMetaEnum>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>

#include "keymap.h"

QString KeyMap::s_keyMapPath = "";

const QString& KeyMap::getKeyMapPath()
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
    if(jsonError.error != QJsonParseError::NoError) {
        errorString = QString("json error: %1").arg(jsonError.errorString());
        goto parseError;
    }
    rootObj = jsonDoc.object();

    // switchKey
    if (!checkItemString(rootObj, "switchKey")) {
        errorString = QString("json error: no find switchKey");
        goto parseError;
    }
    switchKey = getItemKey(rootObj, "switchKey");
    if(switchKey.first == AT_INVALID) {
        errorString = QString("json error: switchKey invalid");
        goto parseError;
    }
    m_switchKey.type = switchKey.first;
    m_switchKey.key= switchKey.second;

    // mouseMoveMap
    if (checkItemObject(rootObj, "mouseMoveMap")) {
        QJsonObject node = rootObj.value("mouseMoveMap").toObject();
        if(!checkForMouseMove(node)){
            if (!checkItemDouble(node, "speedRatio"))
                errorString += QString("json error: mouseMoveMap on find speedRatio\n");
            if (!checkItemObject(node, "startPos"))
                errorString += QString("json error: mouseMoveMap on find startPos");
            goto parseError;
        }
        KeyMapNode keyMapNode = generateMKNMouseMove(node);

        m_idxMouseMove = m_keyMapNodes.size();
        m_keyMapNodes.push_back(keyMapNode);
    }

    // keyMapNodes
    if (rootObj.contains("keyMapNodes") && rootObj.value("keyMapNodes").isArray()) {
        QJsonArray keyMapNodes = rootObj.value("keyMapNodes").toArray();
        int size = keyMapNodes.size();
        for (int i = 0; i < size; i++) {
            if (!keyMapNodes.at(i).isObject()) {
                errorString = QString("json error: keyMapNodes node must be json object");
                goto parseError;
            }
            QJsonObject node = keyMapNodes.at(i).toObject();
            if (!node.contains("type") || !node.value("type").isString()) {
                errorString = QString("json error: keyMapNodes node type not found");
                goto parseError;
            }

            KeyMap::KeyMapType type = getItemKeyMapType(node, "type");
            switch (type) {
            case KeyMap::KMT_CLICK:
            {
                // safe check
                if (!checkForClick(node)) {
                    qWarning() << "json error: format error of type: CLICK, key: " << getItemString(node, "key");
                    break;
                }
                KeyMapNode keyMapNode = generateMKNClick(node);
                if (keyMapNode.type == KMT_INVALID) {
                    break;
                }
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            case KeyMap::KMT_CLICK_TWICE:
            {
                // safe check
                if (!checkForClickTwice(node)) {
                    qWarning() << "json error: format error of type: CLICK_TWICE, key: " << getItemString(node, "key");
                    break;
                }
                KeyMapNode keyMapNode = generateMKNClickTwice(node);
                if (keyMapNode.type == KMT_INVALID) {
                    break;
                }
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            case KeyMap::KMT_STEER_WHEEL:
            {
                // safe check
                if (!checkForSteerWhell(node)) {
                    qWarning() << "json error: format error of type: STEER_WHEEL";
                    break;
                }
                KeyMapNode keyMapNode = generateMKNSteerWheel(node);
                if (keyMapNode.type == KMT_INVALID) {
                    break;
                }
                m_idxSteerWheel = m_keyMapNodes.size();
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            case KeyMap::KMT_DRAG:
            {
                // safe check
                if (!checkForDrag(node)) {
                    qWarning() << "json error: format error of type: DRAG, key: " << getItemString(node, "key");
                    break;
                }
                KeyMapNode keyMapNode = generateMKNDrag(node);
                if (keyMapNode.type == KMT_INVALID) {
                    break;
                }
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            case KeyMap::KMT_FREE_LOOK:
            {
                // safe check
                if (!checkForFreeLook(node)) {
                    qWarning() << "json error: format error of type: FREE_LOOK, key: " << getItemString(node, "key");
                    break;
                }
                KeyMapNode keyMapNode = generateMKNFreeLook(node);
                if (keyMapNode.type == KMT_INVALID) {
                    break;
                }
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            default:
                qWarning() << "json error: invalid node type:" << node.value("type").toString();
                break;
            }
        }
    }
    // this must be called after m_keyMapNodes is stable
    makeReverseMap();
    qInfo() << "Script updated.";

parseError:
    if (!errorString.isEmpty()) {
        qWarning() << errorString;
    }
    return;
}

KeyMap::KeyMapNode KeyMap::generateMKNMouseMove(const QJsonObject& node)
{
    KeyMapNode keyMapNode;
    keyMapNode.type = KMT_MOUSE_MOVE;
    keyMapNode.data.mouseMove.startPos = getItemPos(node, "startPos");
    keyMapNode.data.mouseMove.speedRatio = getItemDouble(node, "speedRatio", 1.0);
    return keyMapNode;
}

KeyMap::KeyMapNode KeyMap::generateMKNClick(const QJsonObject& node)
{
    KeyMapNode keyMapNode;
    KeyNode key = getItemNodeCommon(node, "key", "pos");
    if (key.type == AT_INVALID) {
        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
    }else{
        keyMapNode.type = KMT_CLICK;
        keyMapNode.data.click.keyNode = key;
        keyMapNode.data.click.switchMap = getItemBool(node, "switchMap", false);
    }
    return keyMapNode;
}

KeyMap::KeyMapNode KeyMap::generateMKNClickTwice(const QJsonObject& node)
{
    KeyMapNode keyMapNode;
    KeyNode key = getItemNodeCommon(node, "key", "pos");
    if (key.type == AT_INVALID) {
        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
    }else{
        keyMapNode.type = KMT_CLICK_TWICE;
        keyMapNode.data.click.keyNode = key;
        keyMapNode.data.click.switchMap = false;
    }
    return keyMapNode;
}

KeyMap::KeyMapNode KeyMap::generateMKNSteerWheel(const QJsonObject& node)
{
    KeyMapNode keyMapNode;
    QPair<ActionType, int> leftKey = getItemKey(node, "leftKey");
    QPair<ActionType, int> rightKey = getItemKey(node, "rightKey");
    QPair<ActionType, int> upKey = getItemKey(node, "upKey");
    QPair<ActionType, int> downKey = getItemKey(node, "downKey");
    if (leftKey.first == AT_INVALID || rightKey.first == AT_INVALID
            || upKey.first == AT_INVALID || downKey.first == AT_INVALID) {
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
    }else{
        keyMapNode.type = KMT_STEER_WHEEL;

        keyMapNode.data.steerWheel.left = { leftKey.first, leftKey.second,
                                       getItemDouble(node, "leftOffset") };
        keyMapNode.data.steerWheel.right = { rightKey.first, rightKey.second,
                                        getItemDouble(node, "rightOffset") };
        keyMapNode.data.steerWheel.up = { upKey.first, upKey.second,
                                     getItemDouble(node, "upOffset") };
        keyMapNode.data.steerWheel.down = { downKey.first, downKey.second,
                                       getItemDouble(node, "downOffset") };

        keyMapNode.data.steerWheel.centerPos = getItemPos(node, "centerPos");
    }
    return keyMapNode;
}

KeyMap::KeyMapNode KeyMap::generateMKNDrag(const QJsonObject& node)
{
    KeyMapNode keyMapNode;
    KeyNode key = getItemNodeCommon(node, "key", "startPos");
    if (key.type == AT_INVALID) {
        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
    }else{
        keyMapNode.type = KMT_DRAG;
        keyMapNode.data.click.keyNode = key;
        keyMapNode.data.drag.extendPos = getItemPos(node, "endPos");
    }
    return keyMapNode;
}

KeyMap::KeyMapNode KeyMap::generateMKNFreeLook(const QJsonObject& node)
{
    KeyMapNode keyMapNode;
    KeyNode key = getItemNodeCommon(node, "key", "pos");
    if (key.type == AT_INVALID) {
        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
    }else{
        keyMapNode.type = KMT_FREE_LOOK;
        keyMapNode.data.freeLook.keyNode = key;
        keyMapNode.data.freeLook.speedRatio= getItemDouble(node, "speedRatio", 1.0);
    }
    return keyMapNode;
}

const KeyMap::KeyMapNode& KeyMap::getKeyMapNode(int key)
{
    auto p = m_rmapKey.value(key, &m_invalidNode);
    if (p == &m_invalidNode) {
        return *m_rmapMouse.value(key, &m_invalidNode);
    }
    return *p;
}

const KeyMap::KeyMapNode& KeyMap::getKeyMapNodeKey(int key)
{
    return *m_rmapKey.value(key, &m_invalidNode);
}

const KeyMap::KeyMapNode& KeyMap::getKeyMapNodeMouse(int key)
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

const KeyMap::KeyMapNode& KeyMap::getMouseMoveMap()
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
    for (int i = 0 ; i < m_keyMapNodes.size(); ++i) {
        auto& node = m_keyMapNodes[i];
        switch (node.type) {
        case KMT_CLICK:
        {
            QMultiHash<int, KeyMapNode*>& m = node.data.click.keyNode.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            m.insert(node.data.click.keyNode.key, &node);
        }
            break;
        case KMT_CLICK_TWICE:
        {
            QMultiHash<int, KeyMapNode*>& m = node.data.clickTwice.keyNode.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            m.insert(node.data.clickTwice.keyNode.key, &node);
        }
            break;
        case KMT_STEER_WHEEL:
        {
            QMultiHash<int, KeyMapNode*>& ml = node.data.steerWheel.left.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            ml.insert(node.data.steerWheel.left.key, &node);
            QMultiHash<int, KeyMapNode*>& mr = node.data.steerWheel.right.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            mr.insert(node.data.steerWheel.right.key, &node);
            QMultiHash<int, KeyMapNode*>& mu = node.data.steerWheel.up.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            mu.insert(node.data.steerWheel.up.key, &node);
            QMultiHash<int, KeyMapNode*>& md = node.data.steerWheel.down.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            md.insert(node.data.steerWheel.down.key, &node);
        }
            break;
        case KMT_DRAG:
        {
            QMultiHash<int, KeyMapNode*>& m = node.data.drag.keyNode.type == AT_KEY ? m_rmapKey : m_rmapMouse;
            m.insert(node.data.drag.keyNode.key, &node);
        }
            break;
        default:
            break;
        }
    }
}

QString KeyMap::getItemString(const QJsonObject &node, const QString &name, const QString& def)
{
    return node.value(name).toString(def);
}

double KeyMap::getItemDouble(const QJsonObject& node, const QString& name, const double def)
{
    return node.value(name).toDouble(def);
}

bool KeyMap::getItemBool(const QJsonObject& node, const QString& name, const bool def)
{
    return node.value(name).toBool(def);
}

KeyMap::KeyNode KeyMap::getItemNodeCommon(
        const QJsonObject& node, const QString& keyName, const QString& posName)
{
    KeyNode res;
    QPair<KeyMap::ActionType, int> kp = getItemKey(node, keyName);
    res.type = kp.first;
    res.key = kp.second;
    if(checkItemPos(node, posName)){
        res.pos = getItemPos(node, posName);
    }else{
        res.pos = QPointF(0.0, 0.0);
    }
    return res;
}

QPointF KeyMap::getItemPos(const QJsonObject& node, const QString& name)
{
    QJsonObject pos = node.value(name).toObject();
    return QPointF(pos.value("x").toDouble(), pos.value("y").toDouble());
}

QPair<KeyMap::ActionType, int> KeyMap::getItemKey(const QJsonObject& node, const QString& name)
{
    QString value = getItemString(node, name);
    int key = m_metaEnumKey.keyToValue(value.toStdString().c_str());
    int btn = m_metaEnumMouseButtons.keyToValue(value.toStdString().c_str());
    if (key == -1 && btn == -1) {
        return {AT_INVALID, -1};
    } else if (key != -1) {
        return {AT_KEY, key};
    } else {
        return {AT_MOUSE, btn};
    }
}

KeyMap::KeyMapType KeyMap::getItemKeyMapType(const QJsonObject& node, const QString& name)
{
    QString value = getItemString(node, name);
    return static_cast<KeyMap::KeyMapType>(m_metaEnumKeyMapType.keyToValue(value.toStdString().c_str()));
}

bool KeyMap::checkItemString(const QJsonObject& node, const QString& name)
{
    return node.contains(name) && node.value(name).isString();
}

bool KeyMap::checkItemDouble(const QJsonObject& node, const QString& name)
{
    return node.contains(name) && node.value(name).isDouble();
}

bool KeyMap::checkItemBool(const QJsonObject& node, const QString& name)
{
    return node.contains(name) && node.value(name).isBool();
}

bool KeyMap::checkItemObject(const QJsonObject &node, const QString &name)
{
    return node.contains(name) && node.value(name).isObject();
}

bool KeyMap::checkItemPos(const QJsonObject& node, const QString& name)
{
    if (node.contains(name) && node.value(name).isObject()) {
        QJsonObject pos = node.value(name).toObject();
        return pos.contains("x") && pos.value("x").isDouble()
                && pos.contains("y") && pos.value("y").isDouble();
    }
    return false;
}

bool KeyMap::checkForMouseMove(const QJsonObject& node)
{
    return checkItemDouble(node, "speedRatio") && checkItemPos(node, "startPos");
}

bool KeyMap::checkForClick(const QJsonObject& node)
{
    return checkForClickTwice(node);// && checkItemBool(node, "switchMap");
}

bool KeyMap::checkForClickTwice(const QJsonObject& node)
{
    return checkItemString(node, "key") && checkItemPos(node, "pos");
}

bool KeyMap::checkForSteerWhell(const QJsonObject& node)
{
    return checkItemPos(node, "centerPos")
            && checkItemString(node, "leftKey") && checkItemString(node, "rightKey")
            && checkItemString(node, "upKey") && checkItemString(node, "downKey")
            && checkItemDouble(node, "leftOffset") && checkItemDouble(node, "rightOffset")
            && checkItemDouble(node, "upOffset") && checkItemDouble(node, "downOffset");
}

bool KeyMap::checkForDrag(const QJsonObject& node)
{
    return checkItemString(node, "key")
            && checkItemPos(node, "startPos") && checkItemPos(node, "endPos");
}

bool KeyMap::checkForFreeLook(const QJsonObject& node)
{
    return checkItemString(node, "key")
            && checkItemPos(node, "pos") && checkItemDouble(node, "speedRatio");
}

