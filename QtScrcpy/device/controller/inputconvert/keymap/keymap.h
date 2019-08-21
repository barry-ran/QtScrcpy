#ifndef KEYMAP_H
#define KEYMAP_H
#include <QPointF>
#include <QVector>


class KeyMap
{
public:
    enum KeyMapType {
        KMT_INVALID = -1,
        KMT_CLICK = 0,
        KMT_CLICK_TWICE,
    };

    struct KeyNode {
        int key = Qt::Key_unknown;
        QPointF pos = QPointF(0, 0);
    };

    struct KeyMapNode {
        KeyMapType type = KMT_INVALID;
        union {
            struct {
                KeyNode keyNode;
                bool quitMap = false;
            } click;
            struct {
                KeyNode keyNode;
                bool quitMap = false;
            } clickTwice;
        };
        KeyMapNode() {}
        ~KeyMapNode() {}
    };

    KeyMap();

    void loadKeyMapNode();
    KeyMap::KeyMapNode getKeyMapNode(int key);

private:
    QVector<KeyMapNode> m_keyMapNodes;

};

#endif // KEYMAP_H
