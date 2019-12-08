#include <QDebug>
#include <QCursor>
#include <QGuiApplication>

#include "inputconvertgame.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windef.h>
// restrict mouse into a window
static void restrictMouse(const int left, const int right,
                          const int top, const int bottom)
{
    RECT mainWinRect; // RECT is defined in <windef.h>
    mainWinRect.left = static_cast<LONG>(left);
    mainWinRect.right = static_cast<LONG>(right);
    mainWinRect.top = static_cast<LONG>(top);
    mainWinRect.bottom = static_cast<LONG>(bottom);
    ClipCursor(&mainWinRect); // Windows API
}
static void freeMouse()
{
    ClipCursor(nullptr);
}

#else // linux and macos
static void restrictMouse(const int left, const int right,
                          const int top, const int bottom)
{}
static void freeMouse()
{}
#endif

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

    KeyMap::KeyMapNode& node = m_keyMap.getKeyMapNodeKey(from->key());
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
        case KeyMap::KMT_DRAG:
            processKeyDrag(node.drag.startPos, node.drag.endPos, from);
            return;
        default:
            break;
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
    //QMetaEnum map = QMetaEnum::fromType<Qt::Key>();
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (0 == multiTouchID[i]) {
            multiTouchID[i] = key;
            //qDebug() << "attach "<<key<<" ("<<map.valueToKey(key)<<") as "<<i;
            return i;
        }
    }
    //qDebug() << "attach "<<key<<" ("<<map.valueToKey(key)<<") failed ";
    return -1;
}

void InputConvertGame::detachTouchID(int key)
{
    //QMetaEnum map = QMetaEnum::fromType<Qt::Key>();
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == multiTouchID[i]) {
            multiTouchID[i] = 0;
            //qDebug() << "detach "<<key<<" ("<<map.valueToKey(key)<<") from "<<i;
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

// -------- steer wheel event --------

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
    if (from->key() == node.steerWheel.up.key) {
        node.steerWheel.upKeyPressed = keyPress;
    } else if (from->key() == node.steerWheel.right.key) {
        node.steerWheel.rightKeyPressed = keyPress;
    } else if (from->key() == node.steerWheel.down.key) {
        node.steerWheel.downKeyPressed = keyPress;
    } else if (from->key() == node.steerWheel.left.key) {
        node.steerWheel.leftKeyPressed = keyPress;
    }

    int count = 0;
    keyPress1 = Qt::Key_unknown;
    keyPress2 = Qt::Key_unknown;

    // 上右下左的顺序统计按键数量，并记录前两个按键码
    if (node.steerWheel.upKeyPressed) {
        count++;
        if (Qt::Key_unknown == keyPress1) {
            keyPress1 = node.steerWheel.up.key;
        } else if (Qt::Key_unknown == keyPress2) {
            keyPress2 = node.steerWheel.up.key;
        }
    }
    if (node.steerWheel.rightKeyPressed) {
        count++;
        if (Qt::Key_unknown == keyPress1) {
            keyPress1 = node.steerWheel.right.key;
        } else if (Qt::Key_unknown == keyPress2) {
            keyPress2 = node.steerWheel.right.key;
        }
    }
    if (node.steerWheel.downKeyPressed) {
        count++;
        if (Qt::Key_unknown == keyPress1) {
            keyPress1 = node.steerWheel.down.key;
        } else if (Qt::Key_unknown == keyPress2) {
            keyPress2 = node.steerWheel.down.key;
        }
    }
    if (node.steerWheel.leftKeyPressed) {
        count++;
        if (Qt::Key_unknown == keyPress1) {
            keyPress1 = node.steerWheel.left.key;
        } else if (Qt::Key_unknown == keyPress2) {
            keyPress2 = node.steerWheel.left.key;
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
    if (keyPress1 == node.steerWheel.up.key) {
        movePos.setY(movePos.y() - node.steerWheel.up.offset);
    } else if (keyPress1 == node.steerWheel.right.key) {
        movePos.setX(movePos.x() + node.steerWheel.right.offset);
    } else if (keyPress1 == node.steerWheel.down.key) {
        movePos.setY(movePos.y() + node.steerWheel.down.offset);
    } else if (keyPress1 == node.steerWheel.left.key) {
        movePos.setX(movePos.x() - node.steerWheel.left.offset);
    }
    if(keysNum > 1) {
        if (keyPress2 == node.steerWheel.up.key) {
            movePos.setY(movePos.y() - node.steerWheel.up.offset);
        } else if (keyPress2 == node.steerWheel.right.key) {
            movePos.setX(movePos.x() + node.steerWheel.right.offset);
        } else if (keyPress2 == node.steerWheel.down.key) {
            movePos.setY(movePos.y() + node.steerWheel.down.offset);
        } else if (keyPress2 == node.steerWheel.left.key) {
            movePos.setX(movePos.x() - node.steerWheel.left.offset);
        }
    }
    sendTouchMoveEvent(getTouchID(node.steerWheel.firstPressKey), movePos);
}

// -------- key event --------

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

void InputConvertGame::processKeyDrag(QPointF startPos, QPointF endPos, const QKeyEvent* from)
{
    if(QEvent::KeyPress == from->type()){
        int id = attachTouchID(from->key());
        sendTouchDownEvent(id, startPos);
        sendTouchMoveEvent(id, endPos);
        sendTouchUpEvent(id, endPos);
        detachTouchID(from->key());
    }
}

// -------- mouse event --------

bool InputConvertGame::processMouseClick(const QMouseEvent *from)
{
    KeyMap::KeyMapNode& node = m_keyMap.getKeyMapNodeMouse(from->button());
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
    QPointF mouseMoveStartPos = m_keyMap.getMouseMoveMap().startPos;
    //qreal xbound = qMin(mmsp.x()-0.05, 0.95-mmsp.x());//m_keyMap.getMouseMoveMap().speedRatio;
    //qreal ybound = qMin(mmsp.y()-0.05, 0.95-mmsp.y());//m_keyMap.getMouseMoveMap().speedRatio;

    if(m_mouseMoving){
        QPointF mousePos = from->localPos();
        mousePos.rx() /= m_showSize.width();
        mousePos.ry() /= m_showSize.height();
        QPointF offset = mousePos - mouseMoveStartPos;
        //qDebug()<<from->localPos()<<" - "<<m_mouseMoveLastConverPos<<" - "<<offset<<" - "<<offset.manhattanLength();

        //if (qAbs(offset.x()) > xbound || qAbs(offset.y()) > ybound)
        if(mousePos.x()<0.05 || mousePos.x()>0.95 || mousePos.y()<0.05 || mousePos.y()>0.95)
        {
            //qDebug()<<"reset";
            mouseMoveStopTouch();
            mouseMoveStartTouch(from);
        }
        offset /= m_keyMap.getMouseMoveMap().speedRatio;
        m_mouseMoveLastConverPos = mouseMoveStartPos + offset;
        mouseMoveMovingTouch(m_mouseMoveLastConverPos);
    }else{
        mouseMoveStartTouch(from);
        int left = from->globalX() - from->x();
        int top = from->globalY() - from->y();
        restrictMouse(left, left + m_showSize.width(), top, top+m_showSize.height());
    }
    return true;
}

void InputConvertGame::moveCursorToStart(const QMouseEvent *from)
{
    QPointF mouseMoveStartPosF = m_keyMap.getMouseMoveMap().startPos;
    QPointF mmspLocal = QPointF(
            m_showSize.width()*mouseMoveStartPosF.x(),
            m_showSize.height()*mouseMoveStartPosF.y());
    moveCursorTo(from, mmspLocal.toPoint());
}

void InputConvertGame::moveCursorTo(const QMouseEvent *from, const QPoint &localPos)
{
    QPoint posOffset = from->pos() - localPos;
    QPoint globalPos = from->globalPos();
    globalPos -= posOffset;
    //qDebug()<<"move cursor to "<<globalPos<<" offset "<<posOffset;
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
    if (!m_mouseMoving) {
        QPointF mouseMoveStartPos = m_keyMap.getMouseMoveMap().startPos;
        moveCursorToStart(from);
        int id = attachTouchID(Qt::ExtraButton24);
        sendTouchDownEvent(id, mouseMoveStartPos);
        m_mouseMoveLastConverPos = mouseMoveStartPos;
        m_mouseMoving = true;
    }
}

void InputConvertGame::mouseMoveMovingTouch(const QPointF& target)
{
    if (m_mouseMoving) {
        sendTouchMoveEvent(getTouchID(Qt::ExtraButton24), target);
    }
}

void InputConvertGame::mouseMoveStopTouch()
{
    if (m_mouseMoving) {
        int id = getTouchID(Qt::ExtraButton24);
        sendTouchUpEvent(id, m_mouseMoveLastConverPos);
        detachTouchID(Qt::ExtraButton24);
        m_mouseMoving = false;
    }
}

bool InputConvertGame::switchGameMap()
{
    m_gameMap = !m_gameMap;    
    emit grabCursor(m_gameMap);
    if (m_gameMap) {
        #ifdef QT_NO_DEBUG
            QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
        #else
            QGuiApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
        #endif
        //restrictMouse(); // called at the first run of processMouseMove()
    } else {
        mouseMoveStopTouch();
        QGuiApplication::restoreOverrideCursor();
        freeMouse();
    }
    return m_gameMap;
}

void InputConvertGame::timerEvent(QTimerEvent *event)
{
    if (m_mouseMoveTimer == event->timerId()) {
        stopMouseMoveTimer();
        mouseMoveStopTouch();
    }
}
