#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaEnum>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>

#include "keymap.h"

QString KeyMap::s_keyMapPath = "";

KeyMap::KeyMap(QObject *parent)
    : QObject(parent)
{

}

KeyMap::~KeyMap()
{

}

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

    jsonDoc = QJsonDocument::fromJson(json.toUtf8(), &jsonError);

    if(jsonError.error != QJsonParseError::NoError)
    {
        errorString = QString("json error: %1").arg(jsonError.errorString());
        goto parseError;
    }

    // switchKey
    rootObj = jsonDoc.object();
    if (rootObj.contains("switchKey") && rootObj.value("switchKey").isString()) {
        QPair<ActionType, int> p = getItemKey(rootObj, "switchKey");
        if(p.first == AT_INVALID){
            errorString = QString("json error: switchKey invalid");
            goto parseError;
        }
        m_switchType = p.first;
        m_switchKey = p.second;
    } else {
        errorString = QString("json error: no find switchKey");
        goto parseError;
    }

    // mouseMoveMap
    if (rootObj.contains("mouseMoveMap") && rootObj.value("mouseMoveMap").isObject()) {
        QJsonObject mouseMoveMap = rootObj.value("mouseMoveMap").toObject();
        if (mouseMoveMap.contains("speedRatio") && mouseMoveMap.value("speedRatio").isDouble()) {
            m_mouseMoveMap.speedRatio = mouseMoveMap.value("speedRatio").toInt();
        } else {
            errorString = QString("json error: mouseMoveMap on find speedRatio");
            goto parseError;
        }
        if (mouseMoveMap.contains("startPos") && mouseMoveMap.value("startPos").isObject()) {
            QJsonObject startPos = mouseMoveMap.value("startPos").toObject();
            if (startPos.contains("x") && startPos.value("x").isDouble()) {
                m_mouseMoveMap.startPos.setX(startPos.value("x").toDouble());
            }
            if (startPos.contains("y") && startPos.value("y").isDouble()) {
                m_mouseMoveMap.startPos.setY(startPos.value("y").toDouble());
            }
        } else {
            errorString = QString("json error: mouseMoveMap on find startPos");
            goto parseError;
        }
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

            KeyMap::KeyMapType type = getItemType(node, "type");
            switch (type) {
            case KeyMap::KMT_CLICK:
            {
                // safe check
                if (!checkForClick(node)) {
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }
                QPair<ActionType, int> key = getItemKey(node, "key");
                if(key.first == AT_INVALID){
                    qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
                    break;
                }
                KeyMapNode keyMapNode;
                keyMapNode.type = type;
                keyMapNode.click.keyNode.type = key.first;
                keyMapNode.click.keyNode.key = key.second;
                keyMapNode.click.keyNode.pos = getItemPos(node, "pos");
                keyMapNode.click.switchMap = getItemSwitchMap(node, "switchMap");
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            case KeyMap::KMT_CLICK_TWICE:
            {
                // safe check
                if (!checkForClickDouble(node)) {
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }

                QPair<ActionType, int> key = getItemKey(node, "key");
                if(key.first == AT_INVALID){
                    qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
                    break;
                }
                KeyMapNode keyMapNode;
                keyMapNode.type = type;
                keyMapNode.click.keyNode.type = key.first;
                keyMapNode.click.keyNode.key = key.second;
                keyMapNode.click.keyNode.pos = getItemPos(node, "pos");
                keyMapNode.click.switchMap = getItemSwitchMap(node, "switchMap");
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            case KeyMap::KMT_STEER_WHEEL:
            {
                // safe check
                if(!checkForSteerWhell(node)){
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }
                QPair<ActionType, int> leftKey = getItemKey(node, "leftKey");
                QPair<ActionType, int> rightKey = getItemKey(node, "rightKey");
                QPair<ActionType, int> upKey = getItemKey(node, "upKey");
                QPair<ActionType, int> downKey = getItemKey(node, "downKey");
                if(leftKey.first == AT_INVALID || rightKey.first == AT_INVALID
                        || upKey.first == AT_INVALID || downKey.first == AT_INVALID)
                {
                    if(leftKey.first == AT_INVALID)
                        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("leftKey").toString();
                    if(rightKey.first == AT_INVALID)
                        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("rightKey").toString();
                    if(upKey.first == AT_INVALID)
                        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("upKey").toString();
                    if(downKey.first == AT_INVALID)
                        qWarning() << "json error: keyMapNodes node invalid key: " << node.value("downKey").toString();
                    break;
                }

                KeyMapNode keyMapNode;
                keyMapNode.type = type;

                keyMapNode.steerWheel.left = { leftKey.first, leftKey.second,
                                               getItemNumber(node, "leftOffset") };
                keyMapNode.steerWheel.right = { rightKey.first, rightKey.second,
                                                getItemNumber(node, "rightOffset") };
                keyMapNode.steerWheel.up = { upKey.first, upKey.second,
                                             getItemNumber(node, "upOffset") };
                keyMapNode.steerWheel.down = { downKey.first, downKey.second,
                                               getItemNumber(node, "downOffset") };

                keyMapNode.steerWheel.centerPos = getItemPos(node, "centerPos");
                m_idxSteerWheel = m_keyMapNodes.size();
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            case KeyMap::KMT_DRAG:
            {
                // safe check
                if(!checkForDrag(node)){
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }

                QPair<ActionType, int> key = getItemKey(node, "key");
                if(key.first == AT_INVALID){
                    qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
                    break;
                }
                KeyMapNode keyMapNode;
                keyMapNode.type = type;
                keyMapNode.drag.type = key.first;
                keyMapNode.drag.key = key.second;
                keyMapNode.drag.startPos = getItemPos(node, "startPos");
                keyMapNode.drag.endPos = getItemPos(node, "endPos");
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
    qWarning() << "Script updated.";

parseError:
    if (!errorString.isEmpty()) {
        qWarning() << errorString;
    }
    return;
}

const KeyMap::KeyMapNode& KeyMap::getKeyMapNode(int key)
{
    auto p = rmapKey.value(key, &m_invalidNode);
    if(p == &m_invalidNode)
        return *rmapMouse.value(key, &m_invalidNode);
    return *p;
}

const KeyMap::KeyMapNode& KeyMap::getKeyMapNodeKey(int key)
{
    return *rmapKey.value(key, &m_invalidNode);
}

const KeyMap::KeyMapNode& KeyMap::getKeyMapNodeMouse(int key)
{
    return *rmapMouse.value(key, &m_invalidNode);
}

bool KeyMap::isSwitchOnKeyboard()
{
    return m_switchType == AT_KEY;
}

int KeyMap::getSwitchKey()
{
    return m_switchKey;
}

const KeyMap::MouseMoveMap& KeyMap::getMouseMoveMap()
{
    return m_mouseMoveMap;
}

const KeyMap::KeyMapNode& KeyMap::getSteerWheelMap()
{
    return m_keyMapNodes[m_idxSteerWheel];
}

bool KeyMap::isValidMouseMoveMap()
{
    return !m_mouseMoveMap.startPos.isNull();
}

bool KeyMap::isValidSteerWheelMap()
{
    return m_idxSteerWheel != -1;
}

void KeyMap::makeReverseMap()
{
    rmapKey.clear();
    rmapMouse.clear();
    for(int i = 0 ;i < m_keyMapNodes.size(); ++i) {
        auto& node = m_keyMapNodes[i];
        switch (node.type) {
        case KMT_CLICK:
        {
            QMultiHash<int, KeyMapNode*>& m = node.click.keyNode.type == AT_KEY ? rmapKey : rmapMouse;
            m.insert(node.click.keyNode.key, &node);
        }
            break;
        case KMT_CLICK_TWICE:
        {
            QMultiHash<int, KeyMapNode*>& m = node.clickTwice.keyNode.type == AT_KEY ? rmapKey : rmapMouse;
            m.insert(node.clickTwice.keyNode.key, &node);
        }
            break;
        case KMT_STEER_WHEEL:
        {
            QMultiHash<int, KeyMapNode*>& ml = node.steerWheel.left.type == AT_KEY ? rmapKey : rmapMouse;
            ml.insert(node.steerWheel.left.key, &node);
            QMultiHash<int, KeyMapNode*>& mr = node.steerWheel.right.type == AT_KEY ? rmapKey : rmapMouse;
            mr.insert(node.steerWheel.right.key, &node);
            QMultiHash<int, KeyMapNode*>& mu = node.steerWheel.up.type == AT_KEY ? rmapKey : rmapMouse;
            mu.insert(node.steerWheel.up.key, &node);
            QMultiHash<int, KeyMapNode*>& md = node.steerWheel.down.type == AT_KEY ? rmapKey : rmapMouse;
            md.insert(node.steerWheel.down.key, &node);
        }
            break;
        case KMT_DRAG:
        {
            QMultiHash<int, KeyMapNode*>& m = node.drag.type == AT_KEY ? rmapKey : rmapMouse;
            m.insert(node.drag.key, &node);
        }
            break;
        default:
            break;
        }
    }
}

// ---- check and get of json item ----

bool KeyMap::checkItemKey(const QJsonObject& node, const QString& name)
{
    return node.contains(name) && node.value(name).isString();
}

bool KeyMap::checkItemPos(const QJsonObject& node, const QString& name)
{
    if(node.contains(name) && node.value(name).isObject()){
        QJsonObject pos = node.value(name).toObject();
        return pos.contains("x") && pos.value("x").isDouble()
                && pos.contains("y") && pos.value("y").isDouble();
    }
    return false;
}

bool KeyMap::checkItemDouble(const QJsonObject& node, const QString& name)
{
    return node.contains(name) && node.value(name).isDouble();
}

bool KeyMap::checkItemSwitchMap(const QJsonObject& node, const QString& name)
{
    return !node.contains(name) || node.value(name).isBool();
}

KeyMap::KeyMapType KeyMap::getItemType(const QJsonObject& node, const QString& name)
{
    QString value = node.value(name).toString();
    return static_cast<KeyMap::KeyMapType>(m_metaEnumKeyMapType.keyToValue(value.toStdString().c_str()));
}

QPair<KeyMap::ActionType, int> KeyMap::getItemKey(const QJsonObject& node, const QString& name)
{
    QString value = node.value(name).toString();
    int key = m_metaEnumKey.keyToValue(value.toStdString().c_str());
    int btn = m_metaEnumMouseButtons.keyToValue(value.toStdString().c_str());
    if(key == -1 && btn == -1){
        return {AT_INVALID, -1};
    }else if(key != -1){
        return {AT_KEY, key};
    }else{
        return {AT_MOUSE, btn};
    }
}

QPointF KeyMap::getItemPos(const QJsonObject& node, const QString& name)
{
    QJsonObject pos = node.value(name).toObject();
    return QPointF(pos.value("x").toDouble(), pos.value("y").toDouble());
}

double KeyMap::getItemNumber(const QJsonObject& node, const QString& name)
{
    return node.value(name).toDouble();
}

bool KeyMap::getItemSwitchMap(const QJsonObject& node, const QString& name)
{
    return node.value(name).toBool(false);
}


// ---- check for key-map node ----

bool KeyMap::checkForClick(const QJsonObject& node)
{
    return checkItemKey(node, "key") && checkItemPos(node, "pos")
            && checkItemSwitchMap(node, "switchMap");
}

bool KeyMap::checkForClickDouble(const QJsonObject& node)
{
    return checkForClick(node);
}

bool KeyMap::checkForSteerWhell(const QJsonObject& node)
{
    return checkItemKey(node, "leftKey") && checkItemKey(node, "rightKey")
            && checkItemKey(node, "upKey") && checkItemKey(node, "downKey")
            && checkItemDouble(node, "leftOffset") && checkItemDouble(node, "rightOffset")
            && checkItemDouble(node, "upOffset") && checkItemDouble(node, "downOffset")
            && checkItemPos(node, "centerPos");
}

bool KeyMap::checkForDrag(const QJsonObject& node)
{
    return checkItemKey(node, "key")
            && checkItemPos(node, "startPos") && checkItemPos(node, "endPos")
            && checkItemSwitchMap(node, "switchMap");
}

