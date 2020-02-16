#ifndef KEYMAP_H
#define KEYMAP_H
#include <QObject>
#include <QPointF>
#include <QVector>
#include <QRectF>
#include <QPair>
#include <QMetaEnum>
#include <QMultiHash>
#include <QJsonObject>

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
        KMT_MOUSE_MOVE,
        KMT_FREE_LOOK,
    };    
    Q_ENUM(KeyMapType)

    enum ActionType {
        AT_INVALID = -1,
        AT_KEY = 0,
        AT_MOUSE = 1,
    };
    Q_ENUM(ActionType)

    struct KeyNode {
        ActionType type = ActionType::AT_INVALID;
        int key = -1;
        QPointF pos = QPointF(0.0, 0.0);
    };

    struct KeyMapNode {
        KeyMapType type = KMT_INVALID;
        union Data {
            struct {
                QPointF startPos;
                double speedRatio = 1.0;
            } mouseMove;
            struct {
                KeyNode keyNode;
                bool switchMap;
            } click;
            struct {
                KeyNode keyNode;
            } clickTwice;
            struct {
                QPointF centerPos;
                struct Offset {
                    ActionType type;
                    int key;
                    double offset;
                };
                Offset left, right, up, down;
            } steerWheel;
            struct {
                KeyNode keyNode;
                QPointF extendPos = QPointF(0, 0);
            } drag;
            struct {
                KeyNode keyNode;
                double speedRatio = 1.0;
            } freeLook;
            Data(){}
        } data;
        KeyMapNode(){}
    };

    void loadKeyMap(const QString &json);
    const KeyMap::KeyMapNode& getKeyMapNode(int key);
    const KeyMap::KeyMapNode& getKeyMapNodeKey(int key);
    const KeyMap::KeyMapNode& getKeyMapNodeMouse(int key);
    bool isSwitchOnKeyboard();
    int getSwitchKey();

    bool isValidMouseMoveMap();
    bool isValidSteerWheelMap();
    const KeyMap::KeyMapNode& getMouseMoveMap();

    static const QString& getKeyMapPath();

private:
    KeyMapNode generateMKNMouseMove(const QJsonObject& node);
    KeyMapNode generateMKNClick(const QJsonObject& node);
    KeyMapNode generateMKNClickTwice(const QJsonObject& node);
    KeyMapNode generateMKNSteerWheel(const QJsonObject& node);
    KeyMapNode generateMKNDrag(const QJsonObject& node);
    KeyMapNode generateMKNFreeLook(const QJsonObject& node);

private:
    // set up the reverse map from key/event event to keyMapNode
    void makeReverseMap();

    // safe check for base
    bool checkItemString(const QJsonObject& node, const QString& name);
    bool checkItemDouble(const QJsonObject& node, const QString& name);
    bool checkItemBool(const QJsonObject& node, const QString& name);
    bool checkItemObject(const QJsonObject& node, const QString& name);
    bool checkItemPos(const QJsonObject& node, const QString& name);

    // safe check for KeyMapNode
    bool checkForMouseMove(const QJsonObject& node);
    bool checkForClick(const QJsonObject& node);
    bool checkForClickTwice(const QJsonObject& node);
    bool checkForSteerWhell(const QJsonObject& node);
    bool checkForDrag(const QJsonObject& node);
    bool checkForFreeLook(const QJsonObject& node);

    // get keymap elements from json object
    QString getItemString(const QJsonObject& node, const QString& name, const QString& def="");
    double getItemDouble(const QJsonObject& node, const QString& name, const double def=0.0);
    bool getItemBool(const QJsonObject& node, const QString& name, const bool def=false);
    KeyNode getItemNodeCommon(const QJsonObject& node, const QString& keyName="key", const QString& posName="pos");

    QPointF getItemPos(const QJsonObject& node, const QString& name);
    QPair<ActionType, int> getItemKey(const QJsonObject& node, const QString& name);
    KeyMapType getItemKeyMapType(const QJsonObject& node, const QString& name);

private:
    static QString s_keyMapPath;

    QVector<KeyMapNode> m_keyMapNodes;
    KeyNode m_switchKey = { AT_KEY, Qt::Key_QuoteLeft };

    // just for return
    KeyMapNode m_invalidNode;

    // steer wheel index
    int m_idxSteerWheel = -1;

    // mouse move index
    int m_idxMouseMove = -1;

    // mapping of key/mouse event name to index
    QMetaEnum m_metaEnumKey = QMetaEnum::fromType<Qt::Key>();
    QMetaEnum m_metaEnumMouseButtons = QMetaEnum::fromType<Qt::MouseButtons>();
    QMetaEnum m_metaEnumKeyMapType = QMetaEnum::fromType<KeyMap::KeyMapType>();
    // reverse map of key/mouse event
    QMultiHash<int, KeyMapNode*> m_rmapKey;
    QMultiHash<int, KeyMapNode*> m_rmapMouse;
};

#endif // KEYMAP_H
