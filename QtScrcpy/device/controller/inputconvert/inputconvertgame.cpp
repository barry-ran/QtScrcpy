#include <QDebug>
#include <QCursor>
#include <QGuiApplication>

#include "inputconvertgame.h"

#define CURSOR_POS_CHECK 50

InputConvertGame::InputConvertGame(Controller* controller)
    : InputConvertNormal(controller)
{    

}

InputConvertGame::~InputConvertGame()
{

}

void InputConvertGame::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    // 处理开关按键
    if (m_keyMap.isSwitchOnKeyboard() == false &&
            from->type() == QEvent::MouseButtonPress &&
            m_keyMap.getSwitchKey() == from->button())
    {
        if (!switchGameMap()) {
            m_needSwitchGameAgain = false;
        }
        return;
    }

    if (m_gameMap) {
        updateSize(frameSize, showSize);
        // mouse move
        if (m_keyMap.enableMouseMoveMap()) {
            if (processMouseMove(from)) {
                return;
            }
        }
        // mouse click
        if (processMouseClick(from)) {
            return;
        }
    } else {
        InputConvertNormal::mouseEvent(from, frameSize, showSize);
    }
}

void InputConvertGame::wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (m_gameMap) {
        updateSize(frameSize, showSize);
    } else {
        InputConvertNormal::wheelEvent(from, frameSize, showSize);
    }
}

void InputConvertGame::keyEvent(const QKeyEvent *from, const QSize& frameSize, const QSize& showSize)
{
    // 处理开关按键
    if (m_keyMap.isSwitchOnKeyboard() && m_keyMap.getSwitchKey() == from->key()) {
        if (QEvent::KeyPress == from->type()) {
            if (!switchGameMap()) {
                m_needSwitchGameAgain = false;
            }
        }
        return;
    }

    KeyMap::KeyMapNode& node = m_keyMap.getKeyMapNode(from->key());
    // 处理特殊按键：可以在按键映射和普通映射间切换的按键
    if (m_needSwitchGameAgain
            && KeyMap::KMT_CLICK == node.type
            && node.click.switchMap) {
        updateSize(frameSize, showSize);
        // Qt::Key_Tab Qt::Key_M for PUBG mobile
        processKeyClick(node.click.keyNode.pos, false, node.click.switchMap, from);
        return;
    }

    if (m_gameMap) {
        updateSize(frameSize, showSize);
        if (!from || from->isAutoRepeat()) {
            return;
        }

        switch (node.type) {
        // 处理方向盘
        case KeyMap::KMT_STEER_WHEEL:
            processSteerWheel(node, from);
            return;
        // 处理普通按键
        case KeyMap::KMT_CLICK:
            processKeyClick(node.click.keyNode.pos, false, node.click.switchMap, from);
            return;
        case KeyMap::KMT_CLICK_TWICE:
            processKeyClick(node.clickTwice.keyNode.pos, true, false, from);
            return;
        }
    } else {
        InputConvertNormal::keyEvent(from, frameSize, showSize);
    }
}

void InputConvertGame::loadKeyMap(const QString &json)
{
    m_keyMap.loadKeyMap(json);
    if (m_keyMap.enableMouseMoveMap()) {
        m_mouseMoveLastConverPos = m_keyMap.getMouseMoveMap().startPos;
    }
}

void InputConvertGame::updateSize(const QSize &frameSize, const QSize &showSize)
{
    m_frameSize = frameSize;
    m_showSize = showSize;
}

void InputConvertGame::sendTouchDownEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_DOWN);
}

void InputConvertGame::sendTouchMoveEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_MOVE);
}

void InputConvertGame::sendTouchUpEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_UP);
}

void InputConvertGame::sendTouchEvent(int id, QPointF pos, AndroidMotioneventAction action)
{
    if (0 > id || MULTI_TOUCH_MAX_NUM-1 < id) {
        return;
    }
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_INJECT_TOUCH);
    if (!controlMsg) {
        return;
    }
    controlMsg->setInjectTouchMsgData(id, action, QRect(calcFrameAbsolutePos(pos).toPoint(), m_frameSize));
    sendControlMsg(controlMsg);
}

QPointF InputConvertGame::calcFrameAbsolutePos(QPointF relativePos)
{
    QPointF absolutePos;
    absolutePos.setX(m_frameSize.width() * relativePos.x());
    absolutePos.setY(m_frameSize.height() * relativePos.y());
    return absolutePos;
}

int InputConvertGame::attachTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (0 == multiTouchID[i]) {
            multiTouchID[i] = key;
            return i;
        }
    }
    return -1;
}

void InputConvertGame::detachTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == multiTouchID[i]) {
            multiTouchID[i] = 0;
            return;
        }
    }
}

int InputConvertGame::getTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == multiTouchID[i]) {
            return i;
        }
    }
    return -1;
}

void InputConvertGame::processSteerWheel(KeyMap::KeyMapNode &node, const QKeyEvent *from)
{    
    int keyPress1 = Qt::Key_unknown;
    int keyPress2 = Qt::Key_unknown;
    int keysNum = updateSteerWheelKeysPress(node, from, keyPress1, keyPress2);
    bool needMove = false;
    if (QEvent::KeyPress == from->type()) {
        if (1 == keysNum) {
            node.steerWheel.firstPressKey = from->key();
            int id = attachTouchID(node.steerWheel.firstPressKey);
            if (-1 == id) {
                return;
            }
            sendTouchDownEvent(id, node.steerWheel.centerPos);
            needMove = true;
        } else if (2 == keysNum) {
            needMove = true;
        }
    } else if (QEvent::KeyRelease == from->type()){
        if (0 == keysNum) {
            int id = getTouchID(node.steerWheel.firstPressKey);
            sendTouchUpEvent(id, node.steerWheel.centerPos);
            detachTouchID(node.steerWheel.firstPressKey);
            node.steerWheel.firstPressKey = 0;
        } else if (1 == keysNum) {
            needMove = true;
        }
    }
    if (needMove) {
        steerWheelMove(node, keysNum, keyPress1, keyPress2);
    }
}

int InputConvertGame::updateSteerWheelKeysPress(KeyMap::KeyMapNode &node, const QKeyEvent *from, int& keyPress1, int& keyPress2)
{
    bool keyPress = false;
    if (QEvent::KeyPress == from->type()) {
        keyPress = true;
    } else if (QEvent::KeyRelease == from->type()) {
        keyPress = false;
    }
    if (from->key() == node.steerWheel.upKey) {
        node.steerWheel.upKeyPressed = keyPress;
    } else if (from->key() == node.steerWheel.rightKey) {
        node.steerWheel.rightKeyPressed = keyPress;
    } else if (from->key() == node.steerWheel.downKey) {
        node.steerWheel.downKeyPressed = keyPress;
    } else if (from->key() == node.steerWheel.leftKey) {
        node.steerWheel.leftKeyPressed = keyPress;
    }

    int count = 0;
    keyPress1 = Qt::Key_unknown;
    keyPress2 = Qt::Key_unknown;

    // 上右下左的顺序统计按键数量，并记录前两个按键码
    if (node.steerWheel.upKeyPressed) {
        count++;
        if (Qt::Key_unknown == keyPress1) {
            keyPress1 = node.steerWheel.upKey;
        } else if (Qt::Key_unknown == keyPress2) {
            keyPress2 = node.steerWheel.upKey;
        }
    }
    if (node.steerWheel.rightKeyPressed) {
        count++;
        if (Qt::Key_unknown == keyPress1) {
            keyPress1 = node.steerWheel.rightKey;
        } else if (Qt::Key_unknown == keyPress2) {
            keyPress2 = node.steerWheel.rightKey;
        }
    }
    if (node.steerWheel.downKeyPressed) {
        count++;
        if (Qt::Key_unknown == keyPress1) {
            keyPress1 = node.steerWheel.downKey;
        } else if (Qt::Key_unknown == keyPress2) {
            keyPress2 = node.steerWheel.downKey;
        }
    }
    if (node.steerWheel.leftKeyPressed) {
        count++;
        if (Qt::Key_unknown == keyPress1) {
            keyPress1 = node.steerWheel.leftKey;
        } else if (Qt::Key_unknown == keyPress2) {
            keyPress2 = node.steerWheel.leftKey;
        }
    }
    return count;
}

void InputConvertGame::steerWheelMove(KeyMap::KeyMapNode &node, int keysNum, int keyPress1, int keyPress2)
{
    if (1 != keysNum && 2 != keysNum) {
        return;
    }
    QPointF movePos = node.steerWheel.centerPos;
    switch (keysNum) {
    case 2:
        if (keyPress2 == node.steerWheel.upKey) {
            movePos.setY(movePos.y() - node.steerWheel.upOffset);
        } else if (keyPress2 == node.steerWheel.rightKey) {
            movePos.setX(movePos.x() + node.steerWheel.rightOffset);
        } else if (keyPress2 == node.steerWheel.downKey) {
            movePos.setY(movePos.y() + node.steerWheel.downOffset);
        } else if (keyPress2 == node.steerWheel.leftKey) {
            movePos.setX(movePos.x() - node.steerWheel.leftOffset);
        }
    case 1:
        if (keyPress1 == node.steerWheel.upKey) {
            movePos.setY(movePos.y() - node.steerWheel.upOffset);
        } else if (keyPress1 == node.steerWheel.rightKey) {
            movePos.setX(movePos.x() + node.steerWheel.rightOffset);
        } else if (keyPress1 == node.steerWheel.downKey) {
            movePos.setY(movePos.y() + node.steerWheel.downOffset);
        } else if (keyPress1 == node.steerWheel.leftKey) {
            movePos.setX(movePos.x() - node.steerWheel.leftOffset);
        }
        break;
    }
    sendTouchMoveEvent(getTouchID(node.steerWheel.firstPressKey), movePos);
}

void InputConvertGame::processKeyClick(QPointF clickPos, bool clickTwice, bool switchMap, const QKeyEvent *from)
{
    if (switchMap && QEvent::KeyRelease == from->type()) {
        m_needSwitchGameAgain = !m_needSwitchGameAgain;
        switchGameMap();
    }

    if (QEvent::KeyPress == from->type()) {
        int id = attachTouchID(from->key());
        sendTouchDownEvent(id, clickPos);
        if (clickTwice) {
            sendTouchUpEvent(getTouchID(from->key()), clickPos);
            detachTouchID(from->key());
        }
    } else if (QEvent::KeyRelease == from->type()) {
        if (clickTwice) {
            int id = attachTouchID(from->key());
            sendTouchDownEvent(id, clickPos);
        }
        sendTouchUpEvent(getTouchID(from->key()), clickPos);
        detachTouchID(from->key());
    }
}

bool InputConvertGame::processMouseClick(const QMouseEvent *from)
{
    KeyMap::KeyMapNode& node = m_keyMap.getKeyMapNode(from->button());
    if (KeyMap::KMT_INVALID == node.type) {
        return false;
    }

    if (QEvent::MouseButtonPress == from->type() || QEvent::MouseButtonDblClick == from->type()) {
        int id = attachTouchID(from->button());
        sendTouchDownEvent(id, node.click.keyNode.pos);
    } else if (QEvent::MouseButtonRelease == from->type()) {
        sendTouchUpEvent(getTouchID(from->button()), node.click.keyNode.pos);
        detachTouchID(from->button());
    } else {
        return false;
    }
    return true;
}

bool InputConvertGame::processMouseMove(const QMouseEvent *from)
{
    if (QEvent::MouseMove != from->type()) {
        return false;
    }

    //qWarning() << from->localPos() << " - " << from->globalPos();
    if (checkCursorPos(from)) {
        m_mouseMoveLastPos = QPointF(0.0, 0.0);
        return true;
    }

    if (!m_mouseMoveLastPos.isNull()) {
        QPointF distance = from->localPos() - m_mouseMoveLastPos;
        distance /= m_keyMap.getMouseMoveMap().speedRatio;

        mouseMoveStartTouch(from);
        //startMouseMoveTimer();

        m_mouseMoveLastConverPos.setX(m_mouseMoveLastConverPos.x() + distance.x() / m_showSize.width());
        m_mouseMoveLastConverPos.setY(m_mouseMoveLastConverPos.y() + distance.y() / m_showSize.height());

        if (m_mouseMoveLastConverPos.x() < 0.1
                || m_mouseMoveLastConverPos.x() > 0.8
                || m_mouseMoveLastConverPos.y() < 0.1
                || m_mouseMoveLastConverPos.y() > 0.8) {
            mouseMoveStopTouch();
            mouseMoveStartTouch(from);
        }
        //qWarning() << "move: "<<m_mouseMoveLastConverPos;
        sendTouchMoveEvent(getTouchID(Qt::ExtraButton24), m_mouseMoveLastConverPos);
    }
    m_mouseMoveLastPos = from->localPos();
    return true;
}

void InputConvertGame::moveCursorToStart(const QMouseEvent *from)
{
    QPointF mouseMoveStartPos = m_keyMap.getMouseMoveMap().startPos;
    QPoint localPos = QPoint(m_showSize.width()*mouseMoveStartPos.x(),
                             m_showSize.height()*mouseMoveStartPos.y());
    QPoint posOffset = from->localPos().toPoint() - localPos;
    QPoint globalPos = from->globalPos();

    globalPos -= posOffset;
    QCursor::setPos(globalPos);
}

void InputConvertGame::moveCursorTo(const QMouseEvent *from, const QPoint &pos)
{
    QPoint posOffset = from->localPos().toPoint() - pos;
    QPoint globalPos = from->globalPos();

    globalPos -= posOffset;
    qWarning() <<"move to: "<<globalPos;
    QCursor::setPos(globalPos);
}

void InputConvertGame::startMouseMoveTimer()
{
    stopMouseMoveTimer();
    m_mouseMoveTimer = startTimer(1000);
}

void InputConvertGame::stopMouseMoveTimer()
{
    if (0 != m_mouseMoveTimer) {
        killTimer(m_mouseMoveTimer);
        m_mouseMoveTimer = 0;
    }
}

void InputConvertGame::mouseMoveStartTouch(const QMouseEvent* from)
{
    Q_UNUSED(from)
    if (!m_mouseMovePress) {
        QPointF mouseMoveStartPos = m_keyMap.getMouseMoveMap().startPos;
        moveCursorToStart(from);
        int id = attachTouchID(Qt::ExtraButton24);
        sendTouchDownEvent(id, mouseMoveStartPos);
        m_mouseMoveLastConverPos = mouseMoveStartPos;
        m_mouseMovePress = true;
    }
}

void InputConvertGame::mouseMoveStopTouch()
{
    if (m_mouseMovePress) {
        sendTouchUpEvent(getTouchID(Qt::ExtraButton24), m_mouseMoveLastConverPos);
        detachTouchID(Qt::ExtraButton24);
        m_mouseMovePress = false;
    }
}

bool InputConvertGame::switchGameMap()
{
    m_gameMap = !m_gameMap;    
    emit grabCursor(m_gameMap);
    if (m_gameMap) {
        QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
    } else {
        mouseMoveStopTouch();        
        QGuiApplication::restoreOverrideCursor();
    }
    return m_gameMap;
}

bool InputConvertGame::checkCursorPos(const QMouseEvent *from)
{
    bool moveCursor = true;
    QPoint pos = from->pos();
    if (pos.x() < CURSOR_POS_CHECK) {
        pos.setX(m_showSize.width() - CURSOR_POS_CHECK);
    } else if (pos.x() > m_showSize.width() - CURSOR_POS_CHECK) {
        pos.setX(CURSOR_POS_CHECK);
    } else if (pos.y() < CURSOR_POS_CHECK) {
        pos.setY(m_showSize.height() - CURSOR_POS_CHECK);
    } else if (pos.y() > m_showSize.height() - CURSOR_POS_CHECK) {
        pos.setY(CURSOR_POS_CHECK);
    }else{
        moveCursor = false;
    }

    if (moveCursor) {
        moveCursorTo(from, pos);
    }

    return moveCursor;
}

void InputConvertGame::timerEvent(QTimerEvent *event)
{
    if (m_mouseMoveTimer == event->timerId()) {
        stopMouseMoveTimer();
        mouseMoveStopTouch();
    }
}
