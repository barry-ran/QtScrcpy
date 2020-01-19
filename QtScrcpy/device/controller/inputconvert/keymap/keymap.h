#ifndef KEYMAP_H
#define KEYMAP_H
#include <QObject>
#include <QPointF>
#include <QVector>
#include <QRectF>
#include <QPair>
#include <QMetaEnum>
#include <QMultiHash>

class QJsonObject;

class KeyMap : public QObject
{    
    Q_OBJECT
public:
    enum KeyMapType {
        KMT_INVALID = -1,
        KMT_CLICK = 0,
        KMT_CLICK_TWICE,
        KMT_STEER_WHEEL,
        KMT_DRAG,
    };    
    Q_ENUM(KeyMapType)

    enum ActionType {
        AT_INVALID = -1,
        AT_KEY = 0,
        AT_MOUSE = 1,
    };
    Q_ENUM(ActionType)

    struct KeyNode {
        ActionType type = AT_INVALID;
        int key = Qt::Key_unknown;
        QPointF pos = QPointF(0, 0);
    };

    struct KeyMapNode {
        KeyMapType type = KMT_INVALID;
        union {
            struct {
                KeyNode keyNode;
                bool switchMap = false;
            } click;
            struct {
                KeyNode keyNode;
            } clickTwice;
            struct {
                QPointF centerPos = {0.0, 0.0};
                struct DirInfo{
                    ActionType type = AT_KEY; // keyboard/mouse
                    int key = Qt::Key_unknown; // key/button
                    double offset = 0.0;
                };
                DirInfo left, right, up, down;
            } steerWheel;
            struct {
                ActionType type = AT_KEY;
                int key = Qt::Key_unknown;
                QPointF startPos = QPointF(0, 0);
                QPointF endPos = QPointF(0, 0);
            } drag;
        };
        KeyMapNode() {}
        ~KeyMapNode() {}
    };

    struct MouseMoveMap {
        QPointF startPos = {0.0, 0.0};
        int speedRatio = 1;
    };

    KeyMap(QObject *parent = Q_NULLPTR);
    virtual ~KeyMap();

    void loadKeyMap(const QString &json);
    const KeyMap::KeyMapNode& getKeyMapNode(int key);
    const KeyMap::KeyMapNode& getKeyMapNodeKey(int key);
    const KeyMap::KeyMapNode& getKeyMapNodeMouse(int key);
    bool isSwitchOnKeyboard();
    int getSwitchKey();

    bool isValidMouseMoveMap();
    bool isValidSteerWheelMap();
    const MouseMoveMap& getMouseMoveMap();
    const KeyMapNode& getSteerWheelMap();

    static const QString& getKeyMapPath();

private:
    // set up the reverse map from key/event event to keyMapNode
    void makeReverseMap();

    // parse json of the mapping script
    bool checkItemKey(const QJsonObject& node, const QString& name="key");
    bool checkItemPos(const QJsonObject& node, const QString& name="pos");
    bool checkItemDouble(const QJsonObject& node, const QString& name);
    bool checkItemSwitchMap(const QJsonObject& node, const QString& name="switchMap");

    KeyMapType getItemType(const QJsonObject& node, const QString& name="type");
    QPair<ActionType, int> getItemKey(const QJsonObject& node, const QString& name="key");
    QPointF getItemPos(const QJsonObject& node, const QString& name="pos");
    double getItemNumber(const QJsonObject& node, const QString& name);
    bool getItemSwitchMap(const QJsonObject& node, const QString& name="switchMap");

private:
    bool checkForClick(const QJsonObject& node);
    bool checkForClickDouble(const QJsonObject& node);
    bool checkForSteerWhell(const QJsonObject& node);
    bool checkForDrag(const QJsonObject& node);

private:
    QVector<KeyMapNode> m_keyMapNodes;
    KeyMapNode m_invalidNode;
    ActionType m_switchType = AT_KEY;
    int m_switchKey = Qt::Key_QuoteLeft;
    MouseMoveMap m_mouseMoveMap;
    static QString s_keyMapPath;

    int m_idxSteerWheel = -1;

    // mapping of key/mouse event name to index
    QMetaEnum m_metaEnumKey = QMetaEnum::fromType<Qt::Key>();
    QMetaEnum m_metaEnumMouseButtons = QMetaEnum::fromType<Qt::MouseButtons>();
    QMetaEnum m_metaEnumKeyMapType = QMetaEnum::fromType<KeyMap::KeyMapType>();
    // reverse map of key/mouse event
    QMultiHash<int, KeyMapNode*> rmapKey;
    QMultiHash<int, KeyMapNode*> rmapMouse;
};

#endif // KEYMAP_H
