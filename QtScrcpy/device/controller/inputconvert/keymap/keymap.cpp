#include "keymap.h"

KeyMap::KeyMap()
{

}

void KeyMap::loadKeyMapNode()
{
    KeyMapNode node;
    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_Space; // 跳
    node.click.keyNode.pos = QPointF(0.96f, 0.7f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_M; // 地图
    node.click.keyNode.pos = QPointF(0.98f, 0.03f);
    node.click.switchMap = true;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_Tab; // 背包
    node.click.keyNode.pos = QPointF(0.06f, 0.9f);
    node.click.switchMap = true;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_Z; // 趴
    node.click.keyNode.pos = QPointF(0.95f, 0.9f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_C; // 蹲
    node.click.keyNode.pos = QPointF(0.86f, 0.92f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_R; // 换弹
    node.click.keyNode.pos = QPointF(0.795f, 0.93f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_Alt; // 小眼睛
    node.click.keyNode.pos = QPointF(0.8f, 0.31f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_F; // 捡东西1
    node.click.keyNode.pos = QPointF(0.7f, 0.34f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_G; // 捡东西2
    node.click.keyNode.pos = QPointF(0.7f, 0.44f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_H; // 捡东西3
    node.click.keyNode.pos = QPointF(0.7f, 0.54f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_1; // 换枪1
    node.click.keyNode.pos = QPointF(0.45f, 0.9f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_2; // 换枪2
    node.click.keyNode.pos = QPointF(0.55f, 0.9f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_3; // 手雷
    node.click.keyNode.pos = QPointF(0.67f, 0.92f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_4; // 快速打药
    node.click.keyNode.pos = QPointF(0.33f, 0.95f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_5; // 下车
    node.click.keyNode.pos = QPointF(0.92f, 0.4f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_6; // 救人
    node.click.keyNode.pos = QPointF(0.49f, 0.63f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_Shift; // 车加速
    node.click.keyNode.pos = QPointF(0.82f, 0.8f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_X; // 开关门
    node.click.keyNode.pos = QPointF(0.7f, 0.7f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::Key_T; // 舔包
    node.click.keyNode.pos = QPointF(0.72f, 0.26f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::LeftButton; // 开枪
    node.click.keyNode.pos = QPointF(0.86f, 0.72f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    node.type = KMT_CLICK;
    node.click.keyNode.key = Qt::RightButton; // 开镜
    node.click.keyNode.pos = QPointF(0.96f, 0.52f);
    node.click.switchMap = false;
    m_keyMapNodes.push_back(node);

    KeyMapNode node2;
    node2.type = KMT_CLICK_TWICE;
    node2.clickTwice.keyNode.key = Qt::Key_Q; // 左探头
    node2.clickTwice.keyNode.pos = QPointF(0.12f, 0.35f);
    m_keyMapNodes.push_back(node2);

    node2.type = KMT_CLICK_TWICE;
    node2.clickTwice.keyNode.key = Qt::Key_E; // 右探头
    node2.clickTwice.keyNode.pos = QPointF(0.2, 0.35f);
    m_keyMapNodes.push_back(node2);

    // 方向盘
    KeyMapNode node3;
    node3.type = KMT_STEER_WHEEL;
    node3.steerWheel.centerPos = {0.16f, 0.75f};
    node3.steerWheel.leftOffset = 0.1f;
    node3.steerWheel.rightOffset = 0.1f;
    node3.steerWheel.upOffset = 0.27f;
    node3.steerWheel.downOffset = 0.2f;

    node3.steerWheel.leftKey = Qt::Key_A;
    node3.steerWheel.rightKey = Qt::Key_D;
    node3.steerWheel.upKey = Qt::Key_W;
    node3.steerWheel.downKey = Qt::Key_S;

    m_keyMapNodes.push_back(node3);

    m_mouseMoveMap.startPos = QPointF(0.57f, 0.26f);
    m_mouseMoveMap.speedRatio = 10;

    m_switchKey = Qt::Key_QuoteLeft;
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
        case KMT_CLICK_TWICE:
            if (node.clickTwice.keyNode.key == key) {
                retNode = node;
                find = true;
            }
            break;
        case KMT_STEER_WHEEL:
            if (node.steerWheel.leftKey == key
                    || node.steerWheel.rightKey == key
                    || node.steerWheel.upKey == key
                    || node.steerWheel.downKey == key
                    ) {
                retNode = node;
                find = true;
            }
            break;
        default:
            break;
        }

        if (find) {
            break;
        }
    }

    return retNode;
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
