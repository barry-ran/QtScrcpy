#ifndef KEYMAP_H
#define KEYMAP_H
#include <QJsonObject>
#include <QMetaEnum>
#include <QMultiHash>
#include <QObject>
#include <QPair>
#include <QPointF>
#include <QRectF>
#include <QVector>

#define MAX_DELAY_CLICK_NODES 50

class KeyMap : public QObject
{
    Q_OBJECT
public:
    enum KeyMapType
    {
        KMT_INVALID = -1,
        KMT_CLICK = 0,
        KMT_CLICK_TWICE,
        KMT_CLICK_MULTI,
        KMT_STEER_WHEEL,
        KMT_DRAG,
        KMT_MOUSE_MOVE
    };
    Q_ENUM(KeyMapType)

    enum ActionType
    {
        AT_INVALID = -1,
        AT_KEY = 0,
        AT_MOUSE = 1,
    };
    Q_ENUM(ActionType)

    struct DelayClickNode
    {
        int delay = 0;
        QPointF pos = QPointF(0, 0);
    };

    struct KeyNode
    {
        ActionType type = AT_INVALID;
        int key = Qt::Key_unknown;
        QPointF pos = QPointF(0, 0);                           // normal key
        QPointF extendPos = QPointF(0, 0);                     // for drag
        double extendOffset = 0.0;                             // for steerWheel
        DelayClickNode delayClickNodes[MAX_DELAY_CLICK_NODES]; // for multi clicks
        int delayClickNodesCount = 0;

        KeyNode(
            ActionType type = AT_INVALID,
            int key = Qt::Key_unknown,
            QPointF pos = QPointF(0, 0),
            QPointF extendPos = QPointF(0, 0),
            double extendOffset = 0.0)
            : type(type), key(key), pos(pos), extendPos(extendPos), extendOffset(extendOffset)
        {
        }
    };

    struct KeyMapNode
    {
        KeyMapType type = KMT_INVALID;
        union DATA
        {
            struct
            {
                KeyNode keyNode;
                bool switchMap = false;
            } click;
            struct
            {
                KeyNode keyNode;
            } clickTwice;
            struct
            {
                KeyNode keyNode;
            } clickMulti;
            struct
            {
                QPointF centerPos = { 0.0, 0.0 };
                KeyNode left, right, up, down;
            } steerWheel;
            struct
            {
                KeyNode keyNode;
            } drag;
            struct
            {
                QPointF startPos = { 0.0, 0.0 };
                int speedRatio = 1;
                KeyNode smallEyes;
            } mouseMove;
            DATA() {}
            ~DATA() {}
        } data;

        KeyMapNode() {}
        ~KeyMapNode() {}
    };

    KeyMap(QObject *parent = Q_NULLPTR);
    virtual ~KeyMap();

    void loadKeyMap(const QString &json);
    const KeyMap::KeyMapNode &getKeyMapNode(int key);
    const KeyMap::KeyMapNode &getKeyMapNodeKey(int key);
    const KeyMap::KeyMapNode &getKeyMapNodeMouse(int key);
    bool isSwitchOnKeyboard();
    int getSwitchKey();

    bool isValidMouseMoveMap();
    bool isValidSteerWheelMap();
    const KeyMap::KeyMapNode &getMouseMoveMap();

    static const QString &getKeyMapPath();

private:
    // set up the reverse map from key/event event to keyMapNode
    void makeReverseMap();

    // safe check for base
    bool checkItemString(const QJsonObject &node, const QString &name);
    bool checkItemDouble(const QJsonObject &node, const QString &name);
    bool checkItemBool(const QJsonObject &node, const QString &name);
    bool checkItemObject(const QJsonObject &node, const QString &name);
    bool checkItemPos(const QJsonObject &node, const QString &name);

    // safe check for KeyMapNode
    bool checkForClick(const QJsonObject &node);
    bool checkForClickMulti(const QJsonObject &node);
    bool checkForDelayClickNode(const QJsonObject &node);
    bool checkForClickTwice(const QJsonObject &node);
    bool checkForSteerWhell(const QJsonObject &node);
    bool checkForDrag(const QJsonObject &node);

    // get keymap from json object
    QString getItemString(const QJsonObject &node, const QString &name);
    double getItemDouble(const QJsonObject &node, const QString &name);
    bool getItemBool(const QJsonObject &node, const QString &name);
    QJsonObject getItemObject(const QJsonObject &node, const QString &name);
    QPointF getItemPos(const QJsonObject &node, const QString &name);
    QPair<ActionType, int> getItemKey(const QJsonObject &node, const QString &name);
    KeyMapType getItemKeyMapType(const QJsonObject &node, const QString &name);

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
    QMultiHash<int, KeyMapNode *> m_rmapKey;
    QMultiHash<int, KeyMapNode *> m_rmapMouse;
};

#endif // KEYMAP_H
