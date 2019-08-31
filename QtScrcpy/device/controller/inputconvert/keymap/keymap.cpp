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

    QMetaEnum metaEnumKey = QMetaEnum::fromType<Qt::Key>();
    QMetaEnum metaEnumMouseButtons = QMetaEnum::fromType<Qt::MouseButtons>();
    QMetaEnum metaEnumKeyMapType = QMetaEnum::fromType<KeyMap::KeyMapType>();

    jsonDoc = QJsonDocument::fromJson(json.toUtf8(), &jsonError);

    if(jsonError.error != QJsonParseError::NoError)
    {
        errorString = QString("json error: %1").arg(jsonError.errorString());
        goto parseError;
    }

    // switchKey
    rootObj = jsonDoc.object();
    if (rootObj.contains("switchKey") && rootObj.value("switchKey").isString()) {
        Qt::Key key = (Qt::Key)metaEnumKey.keyToValue(rootObj.value("switchKey").toString().toStdString().c_str());
        if (-1 == key) {
            errorString = QString("json error: switchKey invalid");
            goto parseError;
        }
        m_switchKey = key;
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

            KeyMap::KeyMapType type = (KeyMap::KeyMapType)metaEnumKeyMapType.keyToValue(node.value("type").toString().toStdString().c_str());
            switch (type) {
            case KeyMap::KMT_CLICK:
            {
                // safe check
                if (!node.contains("key") || !node.value("key").isString()
                        || !node.contains("pos") || !node.value("pos").isObject()
                        || !node.value("pos").toObject().contains("x") || !node.value("pos").toObject().value("x").isDouble()
                        || !node.value("pos").toObject().contains("y") || !node.value("pos").toObject().value("y").isDouble()
                        || !node.contains("switchMap") || !node.value("switchMap").isBool()
                        ) {
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }

                Qt::Key key = (Qt::Key)metaEnumKey.keyToValue(node.value("key").toString().toStdString().c_str());
                Qt::MouseButtons btn = (Qt::MouseButtons)metaEnumMouseButtons.keyToValue(node.value("key").toString().toStdString().c_str());
                if (-1 == key && -1 == btn) {
                    qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
                    break;
                }

                KeyMapNode keyMapNode;
                keyMapNode.type = type;
                if (key != -1) {
                    keyMapNode.click.keyNode.key = key;
                } else {
                    keyMapNode.click.keyNode.key = btn;
                }
                keyMapNode.click.keyNode.pos = QPointF(node.value("pos").toObject().value("x").toDouble(),
                                                       node.value("pos").toObject().value("y").toDouble());
                keyMapNode.click.switchMap = node.value("switchMap").toBool();
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            case KeyMap::KMT_CLICK_TWICE:
            {
                // safe check
                if (!node.contains("key") || !node.value("key").isString()
                        || !node.contains("pos") || !node.value("pos").isObject()
                        || !node.value("pos").toObject().contains("x") || !node.value("pos").toObject().value("x").isDouble()
                        || !node.value("pos").toObject().contains("y") || !node.value("pos").toObject().value("y").isDouble()
                        ) {
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }

                Qt::Key key = (Qt::Key)metaEnumKey.keyToValue(node.value("key").toString().toStdString().c_str());
                Qt::MouseButtons btn = (Qt::MouseButtons)metaEnumMouseButtons.keyToValue(node.value("key").toString().toStdString().c_str());
                if (-1 == key && -1 == btn) {
                    qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
                    break;
                }

                KeyMapNode keyMapNode;
                keyMapNode.type = type;
                if (key != -1) {
                    keyMapNode.clickTwice.keyNode.key = key;
                } else {
                    keyMapNode.clickTwice.keyNode.key = btn;
                }
                keyMapNode.clickTwice.keyNode.pos = QPointF(node.value("pos").toObject().value("x").toDouble(),
                                                       node.value("pos").toObject().value("y").toDouble());
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            case KeyMap::KMT_STEER_WHEEL:
            {
                // safe check
                if (!node.contains("leftKey") || !node.value("leftKey").isString()
                        || !node.contains("rightKey") || !node.value("rightKey").isString()
                        || !node.contains("upKey") || !node.value("upKey").isString()
                        || !node.contains("downKey") || !node.value("downKey").isString()
                        || !node.contains("leftOffset") || !node.value("leftOffset").isDouble()
                        || !node.contains("rightOffset") || !node.value("rightOffset").isDouble()
                        || !node.contains("upOffset") || !node.value("upOffset").isDouble()
                        || !node.contains("downOffset") || !node.value("downOffset").isDouble()
                        || !node.contains("centerPos") || !node.value("centerPos").isObject()
                        || !node.value("centerPos").toObject().contains("x") || !node.value("centerPos").toObject().value("x").isDouble()
                        || !node.value("centerPos").toObject().contains("y") || !node.value("centerPos").toObject().value("y").isDouble()
                        ) {
                    qWarning() << "json error: keyMapNodes node format error";
                    break;
                }

                Qt::Key leftKey = (Qt::Key)metaEnumKey.keyToValue(node.value("leftKey").toString().toStdString().c_str());
                Qt::MouseButtons leftBtn = (Qt::MouseButtons)metaEnumMouseButtons.keyToValue(node.value("leftKey").toString().toStdString().c_str());
                Qt::Key rightKey = (Qt::Key)metaEnumKey.keyToValue(node.value("rightKey").toString().toStdString().c_str());
                Qt::MouseButtons rightBtn = (Qt::MouseButtons)metaEnumMouseButtons.keyToValue(node.value("rightKey").toString().toStdString().c_str());
                Qt::Key upKey = (Qt::Key)metaEnumKey.keyToValue(node.value("upKey").toString().toStdString().c_str());
                Qt::MouseButtons upBtn = (Qt::MouseButtons)metaEnumMouseButtons.keyToValue(node.value("upKey").toString().toStdString().c_str());
                Qt::Key downKey = (Qt::Key)metaEnumKey.keyToValue(node.value("downKey").toString().toStdString().c_str());
                Qt::MouseButtons downBtn = (Qt::MouseButtons)metaEnumMouseButtons.keyToValue(node.value("downKey").toString().toStdString().c_str());

                if ((-1 == leftKey && -1 == leftBtn)
                        || (-1 == rightKey && -1 == rightBtn)
                        || (-1 == upKey && -1 == upBtn)
                        || (-1 == downKey && -1 == downBtn)
                        ) {
                    qWarning() << "json error: keyMapNodes node invalid key: " << node.value("key").toString();
                    break;
                }

                KeyMapNode keyMapNode;
                keyMapNode.type = type;
                keyMapNode.steerWheel.leftKeyPressed = false;
                keyMapNode.steerWheel.rightKeyPressed = false;
                keyMapNode.steerWheel.upKeyPressed = false;
                keyMapNode.steerWheel.downKeyPressed = false;
                keyMapNode.steerWheel.pressKeysNum = 0;
                keyMapNode.steerWheel.firstPressKey = 0;

                if (leftKey != -1) {
                    keyMapNode.steerWheel.leftKey = leftKey;
                } else {
                    keyMapNode.steerWheel.leftKey = leftBtn;
                }
                if (rightKey != -1) {
                    keyMapNode.steerWheel.rightKey = rightKey;
                } else {
                    keyMapNode.steerWheel.rightKey = rightBtn;
                }
                if (upKey != -1) {
                    keyMapNode.steerWheel.upKey = upKey;
                } else {
                    keyMapNode.steerWheel.upKey = upBtn;
                }
                if (downKey != -1) {
                    keyMapNode.steerWheel.downKey = downKey;
                } else {
                    keyMapNode.steerWheel.downKey = downBtn;
                }
                keyMapNode.steerWheel.leftOffset = node.value("leftOffset").toDouble();
                keyMapNode.steerWheel.rightOffset = node.value("rightOffset").toDouble();
                keyMapNode.steerWheel.upOffset = node.value("upOffset").toDouble();
                keyMapNode.steerWheel.downOffset = node.value("downOffset").toDouble();
                keyMapNode.steerWheel.centerPos = QPointF(node.value("centerPos").toObject().value("x").toDouble(),
                                                       node.value("centerPos").toObject().value("y").toDouble());
                m_keyMapNodes.push_back(keyMapNode);
            }
                break;
            default:
                qWarning() << "json error: keyMapNodes invalid node type:" << node.value("type").toString();
                break;
            }
        }
    }

parseError:
    if (!errorString.isEmpty()) {
        qWarning() << errorString;
    }
    return;
}

KeyMap::KeyMapNode& KeyMap::getKeyMapNode(int key)
{
    for (auto& itemNode : m_keyMapNodes) {
        switch (itemNode.type) {
        case KMT_CLICK:
            if (itemNode.click.keyNode.key == key) {
                return itemNode;
            }
            break;
        case KMT_CLICK_TWICE:
            if (itemNode.clickTwice.keyNode.key == key) {
                return itemNode;
            }
            break;
        case KMT_STEER_WHEEL:
            if (itemNode.steerWheel.leftKey == key
                    || itemNode.steerWheel.rightKey == key
                    || itemNode.steerWheel.upKey == key
                    || itemNode.steerWheel.downKey == key
                    ) {
                return itemNode;
            }
            break;
        default:
            break;
        }
    }

    return m_invalidNode;
}

int KeyMap::getSwitchKey()
{
    return m_switchKey;
}

KeyMap::MouseMoveMap KeyMap::getMouseMoveMap()
{
    return m_mouseMoveMap;
}

bool KeyMap::enableMouseMoveMap()
{
    return !m_mouseMoveMap.startPos.isNull();
}
