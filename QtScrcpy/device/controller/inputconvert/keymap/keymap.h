#ifndef KEYMAP_H
#define KEYMAP_H
#include <QObject>
#include <QPointF>
#include <QVector>
#include <QRectF>


class KeyMap : public QObject
{    
    Q_OBJECT
public:
    enum KeyMapType {
        KMT_INVALID = -1,
        KMT_CLICK = 0,
        KMT_CLICK_TWICE,
        KMT_STEER_WHEEL,
    };    
    Q_ENUM(KeyMapType)

    struct KeyNode {
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
                // 方向盘矩形中心位置                
                QPointF centerPos = {0.0f, 0.0f};

                // 方向盘矩形四个方向偏移量                
                float leftOffset = 0.0f;
                float rightOffset = 0.0f;
                float upOffset = 0.0f;
                float downOffset = 0.0f;

                // 方向盘矩形四个方向按键                
                int leftKey = Qt::Key_unknown;
                int rightKey = Qt::Key_unknown;
                int upKey = Qt::Key_unknown;
                int downKey = Qt::Key_unknown;

                // 辅助变量
                // 方向键的按下状态
                bool leftKeyPressed = false;
                bool rightKeyPressed = false;
                bool upKeyPressed = false;
                bool downKeyPressed = false;
                // 按下方向键的数量
                int pressKeysNum = 0;
                // 第一次按下的键
                int firstPressKey = 0;
            } steerWheel;
        };
        KeyMapNode() {}
        ~KeyMapNode() {}
    };

    struct MouseMoveMap {
        QPointF startPos = {0.0f, 0.0f};
        int speedRatio = 1;
    };

    KeyMap(QObject *parent = Q_NULLPTR);
    virtual ~KeyMap();

    void loadKeyMap(const QString &json);
    KeyMap::KeyMapNode getKeyMapNode(int key);
    int getSwitchKey();
    MouseMoveMap getMouseMoveMap();
    bool enableMouseMoveMap();

protected:
    const QString& getKeyMapPath();

private:
    QVector<KeyMapNode> m_keyMapNodes;
    int m_switchKey = Qt::Key_QuoteLeft;
    MouseMoveMap m_mouseMoveMap;
    static QString s_keyMapPath;
};

#endif // KEYMAP_H
