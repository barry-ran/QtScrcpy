﻿#include <QDebug>
#include <QCursor>
#include <QGuiApplication>

#include "inputconvertgame.h"

static const int CURSOR_POS_CHECK = 50;

InputConvertGame::InputConvertGame(Controller* controller)
    : InputConvertNormal(controller)
{    

}

InputConvertGame::~InputConvertGame()
{

}

void InputConvertGame::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    // handle the switch key
    if (m_keyMap.isSwitchOnKeyboard() == false && m_keyMap.getSwitchKey() == from->button()) {
        if (from->type() == QEvent::MouseButtonPress) {
            if (!switchGameMap()) {
                m_needSwitchGameAgain = false;
            }
        }
        return;
    }

    if(m_gameMap){
        updateSize(frameSize, showSize);
        if (QEvent::MouseMove == from->type() && m_ctrlMouseMove.valid){
            // mouse move event
            processMouseMove(from);
        } else {
            // mouse click event
            const KeyMap::KeyMapNode& node = m_keyMap.getKeyMapNodeMouse(from->button());
            switch(node.type) {
            case KeyMap::KMT_CLICK:
                processClick(node.data.click.keyNode.pos, false, node.data.click.switchMap,
                             from->button(), from->type() == QEvent::MouseButtonPress,
                             from->type() == QEvent::MouseButtonRelease);
                break;
            case KeyMap::KMT_CLICK_TWICE:
                processClick(node.data.clickTwice.keyNode.pos, true, false,
                             from->button(), from->type() == QEvent::MouseButtonPress,
                             from->type() == QEvent::MouseButtonRelease);
                break;
            case KeyMap::KMT_DRAG:
                processDrag(node.data.drag.keyNode.pos, node.data.drag.extendPos,
                            from->button(), from->type() == QEvent::MouseButtonPress,
                            from->type() == QEvent::MouseButtonRelease);
                break;
            case KeyMap::KMT_FREE_LOOK:
                processFreeLook(node.data.freeLook.keyNode.pos, node.data.freeLook.speedRatio,
                            from->type() == QEvent::MouseButtonPress,
                            from->type() == QEvent::MouseButtonRelease);
                break;
            default:
                break;
            }
        }
    }else{
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
    // handle the switch key
    if (m_keyMap.isSwitchOnKeyboard() && m_keyMap.getSwitchKey() == from->key()) {
        if (QEvent::KeyPress == from->type()) {
            if (!switchGameMap()) {
                m_needSwitchGameAgain = false;
            }
        }
        return;
    }

    const KeyMap::KeyMapNode& node = m_keyMap.getKeyMapNodeKey(from->key());
    // 处理特殊按键：可以在按键映射和普通映射间切换的按键
    if (m_needSwitchGameAgain
            && KeyMap::KMT_CLICK == node.type
            && node.data.click.switchMap) {
        updateSize(frameSize, showSize);
        // e.g.: Qt::Key_Tab, Qt::Key_M for PUBG mobile
        processClick(node.data.click.keyNode.pos, false, node.data.click.switchMap,
                     from->key(), from->type() == QEvent::KeyPress, from->type() == QEvent::KeyRelease);
        return;
    }

    if (m_gameMap) {
        updateSize(frameSize, showSize);
        if (!from || from->isAutoRepeat()) {
            return;
        }

        switch (node.type) {
        case KeyMap::KMT_STEER_WHEEL:
            processSteerWheel(node, from->key(), from->type() == QEvent::KeyPress);
            return;
        case KeyMap::KMT_CLICK:
            processClick(node.data.click.keyNode.pos, false, node.data.click.switchMap,
                         from->key(), from->type() == QEvent::KeyPress, from->type() == QEvent::KeyRelease);
            return;
        case KeyMap::KMT_CLICK_TWICE:
            processClick(node.data.clickTwice.keyNode.pos, true, false,
                         from->key(), from->type() == QEvent::KeyPress, from->type() == QEvent::KeyRelease);
            return;
        case KeyMap::KMT_DRAG:
            processDrag(node.data.drag.keyNode.pos, node.data.drag.extendPos,
                        from->key(), from->type() == QEvent::KeyPress, from->type() == QEvent::KeyRelease);
            return;
        case KeyMap::KMT_FREE_LOOK:
            processFreeLook(node.data.freeLook.keyNode.pos, node.data.freeLook.speedRatio,
                            from->type() == QEvent::KeyPress, from->type() == QEvent::KeyRelease);
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
    m_ctrlMouseMove.valid = m_keyMap.isValidMouseMoveMap();
    if (m_ctrlMouseMove.valid) {
        const KeyMap::KeyMapNode& n = m_keyMap.getMouseMoveMap();
        m_ctrlMouseMove.detail.startPos = n.data.mouseMove.startPos;
        m_ctrlMouseMove.detail.speedRatio = n.data.mouseMove.speedRatio;
    }
}

void InputConvertGame::updateSize(const QSize &frameSize, const QSize &showSize)
{
    if (showSize != m_showSize) {
        if (m_gameMap && m_keyMap.isValidMouseMoveMap()) {
            // show size change, resize grab cursor
            emit grabCursor(true);
        }
    }
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
        Q_ASSERT(0);
        return;
    }
    //qDebug() << "id:" << id << " pos:" << pos << " action" << action;
    ControlMsg* controlMsg = new ControlMsg(ControlMsg::CMT_INJECT_TOUCH);
    if (!controlMsg) {
        return;
    }
    controlMsg->setInjectTouchMsgData(static_cast<quint64>(id),
                                      action,
                                      static_cast<AndroidMotioneventButtons>(0),
                                      QRect(calcFrameAbsolutePos(pos).toPoint(),m_frameSize),
                                      1.0f);
    sendControlMsg(controlMsg);
}

QPointF InputConvertGame::calcFrameAbsolutePos(QPointF relativePos)
{
    QPointF absolutePos;
    absolutePos.setX(m_frameSize.width() * relativePos.x());
    absolutePos.setY(m_frameSize.height() * relativePos.y());
    return absolutePos;
}

QPointF InputConvertGame::calcScreenAbsolutePos(QPointF relativePos)
{
    QPointF absolutePos;
    absolutePos.setX(m_showSize.width() * relativePos.x());
    absolutePos.setY(m_showSize.height() * relativePos.y());
    return absolutePos;
}

int InputConvertGame::attachTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (0 == m_multiTouchID[i]) {
            m_multiTouchID[i] = key;
            return i;
        }
    }
    return -1;
}

void InputConvertGame::detachTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == m_multiTouchID[i]) {
            m_multiTouchID[i] = 0;
            return;
        }
    }
}

int InputConvertGame::getTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == m_multiTouchID[i]) {
            return i;
        }
    }
    return -1;
}

// -------- steer wheel event --------

void InputConvertGame::processSteerWheel(
        const KeyMap::KeyMapNode &node, int keyButtonID, bool isPress)
{
    int key = keyButtonID;
    bool flag = isPress;
    // identify keys
    if (key == node.data.steerWheel.up.key) {
        m_ctrlSteerWheel.pressedUp = flag;
    } else if (key == node.data.steerWheel.right.key) {
        m_ctrlSteerWheel.pressedRight = flag;
    } else if (key == node.data.steerWheel.down.key) {
        m_ctrlSteerWheel.pressedDown = flag;
    } else { // left
        m_ctrlSteerWheel.pressedLeft = flag;
    }

    // calc offset and pressed number
    QPointF offset(0.0, 0.0);
    int pressedNum = 0;
    if (m_ctrlSteerWheel.pressedUp) {
        ++pressedNum;
        offset.ry() -= node.data.steerWheel.up.offset;
    }
    if (m_ctrlSteerWheel.pressedRight) {
        ++pressedNum;
        offset.rx() += node.data.steerWheel.right.offset;
    }
    if (m_ctrlSteerWheel.pressedDown) {
        ++pressedNum;
        offset.ry() += node.data.steerWheel.down.offset;
    }
    if (m_ctrlSteerWheel.pressedLeft) {
        ++pressedNum;
        offset.rx() -= node.data.steerWheel.left.offset;
    }

    // action
    if(pressedNum == 0){
        // touch up release all
        int id = getTouchID(m_ctrlSteerWheel.touchKey);
        sendTouchUpEvent(id, node.data.steerWheel.centerPos + m_ctrlSteerWheel.lastOffset);
        detachTouchID(m_ctrlSteerWheel.touchKey);
    } else {
        int id;
        // first press, get key and touch down
        if (pressedNum == 1 && flag) {
            m_ctrlSteerWheel.touchKey = key;
            id = attachTouchID(m_ctrlSteerWheel.touchKey);
            sendTouchDownEvent(id, node.data.steerWheel.centerPos);
        } else {
            // jsut get touch id and move
            id = getTouchID(m_ctrlSteerWheel.touchKey);
        }
        sendTouchMoveEvent(id, node.data.steerWheel.centerPos + offset);
    }
    m_ctrlSteerWheel.lastOffset = offset;
    return;
}

// -------- click event --------

void InputConvertGame::processClick(
        const QPointF& clickPos, bool clickTwice, bool switchMap,
        int keyButtonID, bool isPress, bool isRelease)
{
    if (switchMap && isPress) {
        m_needSwitchGameAgain = !m_needSwitchGameAgain;
        switchGameMap();
    }

    if (isPress) {
        int id = attachTouchID(keyButtonID);
        sendTouchDownEvent(id, clickPos);
        if (clickTwice) {
            sendTouchUpEvent(getTouchID(keyButtonID), clickPos);
            detachTouchID(keyButtonID);
        }
    } else if (isRelease) {
        if (clickTwice) {
            int id = attachTouchID(keyButtonID);
            sendTouchDownEvent(id, clickPos);
        }
        sendTouchUpEvent(getTouchID(keyButtonID), clickPos);
        detachTouchID(keyButtonID);
    }
}

void InputConvertGame::processDrag(
        const QPointF& startPos, QPointF endPos,
        int keyButtonID, bool isPress, bool isRelease)
{
    if (isPress){
        int id = attachTouchID(keyButtonID);
        sendTouchDownEvent(id, startPos);
        sendTouchMoveEvent(id, endPos);
    } else if (isRelease) {
        int id = getTouchID(keyButtonID);
        sendTouchUpEvent(id, endPos);
        detachTouchID(keyButtonID);
    }
}

// -------- free look --------

void InputConvertGame::processFreeLook(
        const QPointF& startPos, const double speedRatio, bool isPress, bool isRelease)
{
    if (isPress){
        // switch to this node's position and sensitivity
        m_ctrlMouseMove.detail.startPos = startPos;
        m_ctrlMouseMove.detail.speedRatio = speedRatio;
    } else if (isRelease){
        // switch back to the mouse-move position and sensitivity
        if (m_keyMap.isValidMouseMoveMap()){
            const KeyMap::KeyMapNode& n = m_keyMap.getMouseMoveMap();
            m_ctrlMouseMove.detail.startPos = n.data.mouseMove.startPos;
            m_ctrlMouseMove.detail.speedRatio = n.data.mouseMove.speedRatio;
        }
    }
    qDebug() <<"free look"<<isPress<<","<<isRelease<<","<<m_ctrlMouseMove.detail.startPos<<","<<m_ctrlMouseMove.detail.speedRatio;
    mouseMoveStopTouch();
}

// -------- mouse move --------

void InputConvertGame::processMouseMove(const QMouseEvent *from)
{
    if (checkCursorPos(from)) {
        m_ctrlMouseMove.lastPos = QPointF(0.0, 0.0);
        return;
    }

    if (!m_ctrlMouseMove.lastPos.isNull()) {
        QPointF distance = from->localPos() - m_ctrlMouseMove.lastPos;
        distance /= m_ctrlMouseMove.detail.speedRatio;

        mouseMoveStartTouch(from);
        startMouseMoveTimer();

        m_ctrlMouseMove.lastConverPos.rx() += distance.x() / m_showSize.width();
        m_ctrlMouseMove.lastConverPos.ry() += distance.y() / m_showSize.height();

        if (m_ctrlMouseMove.lastConverPos.x() < 0.1
                || m_ctrlMouseMove.lastConverPos.x() > 0.8
                || m_ctrlMouseMove.lastConverPos.y() < 0.1
                || m_ctrlMouseMove.lastConverPos.y() > 0.8) {
            mouseMoveStopTouch();
            mouseMoveStartTouch(from);
        }

        sendTouchMoveEvent(getTouchID(Qt::ExtraButton24), m_ctrlMouseMove.lastConverPos);
    }
    m_ctrlMouseMove.lastPos = from->localPos();
}

bool InputConvertGame::checkCursorPos(const QMouseEvent *from)
{
    bool moveCursor = false;
    QPoint pos = from->pos();
    if (pos.x() < CURSOR_POS_CHECK) {
        pos.setX(m_showSize.width() - CURSOR_POS_CHECK);
        moveCursor = true;
    } else if (pos.x() > m_showSize.width() - CURSOR_POS_CHECK) {
        pos.setX(CURSOR_POS_CHECK);
        moveCursor = true;
    } else if (pos.y() < CURSOR_POS_CHECK) {
        pos.setY(m_showSize.height() - CURSOR_POS_CHECK);
        moveCursor = true;
    } else if (pos.y() > m_showSize.height() - CURSOR_POS_CHECK) {
        pos.setY(CURSOR_POS_CHECK);
        moveCursor = true;
    }

    if (moveCursor) {
        moveCursorTo(from, pos);
    }

    return moveCursor;
}

void InputConvertGame::moveCursorTo(const QMouseEvent *from, const QPoint &localPosPixel)
{
    QPoint posOffset = from->pos() - localPosPixel;
    QPoint globalPos = from->globalPos();
    globalPos -= posOffset;
    //qDebug()<<"move cursor to "<<globalPos<<" offset "<<posOffset;
    QCursor::setPos(globalPos);
}

void InputConvertGame::mouseMoveStartTouch(const QMouseEvent* from)
{
    Q_UNUSED(from)
    if (!m_ctrlMouseMove.touching) {
        QPointF mouseMoveStartPos = m_ctrlMouseMove.detail.startPos;
        int id = attachTouchID(Qt::ExtraButton24);
        sendTouchDownEvent(id, mouseMoveStartPos);
        m_ctrlMouseMove.lastConverPos = mouseMoveStartPos;
        m_ctrlMouseMove.touching = true;
    }
}

void InputConvertGame::mouseMoveStopTouch()
{
    if (m_ctrlMouseMove.touching) {
        sendTouchUpEvent(getTouchID(Qt::ExtraButton24), m_ctrlMouseMove.lastConverPos);
        detachTouchID(Qt::ExtraButton24);
        m_ctrlMouseMove.touching = false;
    }
}

void InputConvertGame::startMouseMoveTimer()
{
    stopMouseMoveTimer();
    m_ctrlMouseMove.timer = startTimer(1000);
}

void InputConvertGame::stopMouseMoveTimer()
{
    if (0 != m_ctrlMouseMove.timer) {
        killTimer(m_ctrlMouseMove.timer);
        m_ctrlMouseMove.timer = 0;
    }
}

bool InputConvertGame::switchGameMap()
{
    m_gameMap = !m_gameMap;

    if (!m_keyMap.isValidMouseMoveMap()) {
        return m_gameMap;
    }

    // grab cursor and set cursor only mouse move map
    emit grabCursor(m_gameMap);
    if (m_gameMap) {
#ifdef QT_NO_DEBUG
        QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
#else
        QGuiApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
#endif
    } else {
        QGuiApplication::restoreOverrideCursor();
    }
    return m_gameMap;
}

void InputConvertGame::timerEvent(QTimerEvent *event)
{
    if (m_ctrlMouseMove.timer == event->timerId()) {
        stopMouseMoveTimer();
        mouseMoveStopTouch();
    }
}
