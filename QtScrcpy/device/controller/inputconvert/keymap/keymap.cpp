#include "keymap.h"

KeyMap::KeyMap()
{

}

void KeyMap::loadKeyMapNode()
{
    KeyMapNode node;
    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_J;
    node.click.keyNode.pos = QPointF(0.5f, 0.5f);
    node.click.quitMap = false;

    m_keyMapNodes.push_back(node);
}

KeyMap::KeyMapNode KeyMap::getKeyMapNode(int key)
{
    KeyMapNode retNode;
    bool find = false;
    for (auto& node : m_keyMapNodes) {
        switch (node.type) {
        case KMT_CLICK:
            if (node.click.keyNode.key == key) {
                retNode = node;
                find = true;
            }
            break;
        }

        if (find) {
            break;
        }
    }

    return retNode;
}
