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
        if (m_keyMap.isValidMouseMoveMap()) {
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

    const KeyMap::KeyMapNode& node = m_keyMap.getKeyMapNodeKey(from->key());
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
    if (m_keyMap.isValidMouseMoveMap()) {
        m_ctrlMouseMove.valid = true;
        m_ctrlMouseMove.touching = false;
        m_ctrlMouseMove.startPosRel = m_keyMap.getMouseMoveMap().startPos;
        m_ctrlMouseMove.startPosPixel = calcFrameAbsolutePos(m_ctrlMouseMove.startPosRel);
    }
    if(m_keyMap.isValidSteerWheelMap()){
        m_ctrlSteerWheel.valid = true;
        m_ctrlMouseMove.touching = false;
    }
}

void InputConvertGame::updateSize(const QSize &frameSize, const QSize &showSize)
{
    m_frameSize = frameSize;
    m_showSize = showSize;
    if(m_ctrlMouseMove.valid){
        m_ctrlMouseMove.startPosPixel = calcScreenAbsolutePos(m_ctrlMouseMove.startPosRel);
    }
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
    controlMsg->setInjectTouchMsgData(id, action, (AndroidMotioneventButtons)0, QRect(calcFrameAbsolutePos(pos).toPoint(), m_frameSize), 1.0f);
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

void InputConvertGame::processSteerWheel(const KeyMap::KeyMapNode &node, const QKeyEvent *from)
{
    int key = from->key();
    bool flag = from->type() == QEvent::KeyPress;
    // identify keys
    if(key == node.steerWheel.up.key){
        m_ctrlSteerWheel.pressedUp = flag;
    }else if(key == node.steerWheel.right.key){
        m_ctrlSteerWheel.pressedRight = flag;
    }else if(key == node.steerWheel.down.key){
        m_ctrlSteerWheel.pressedDown = flag;
    }else{ // left
        m_ctrlSteerWheel.pressedLeft = flag;
    }
    QPointF offset(0.0, 0.0);
    int nPressed = 0;
    if(m_ctrlSteerWheel.pressedUp){
        ++nPressed;
        offset.ry() -= node.steerWheel.up.offset;
    }
    if(m_ctrlSteerWheel.pressedRight){
        ++nPressed;
        offset.rx() += node.steerWheel.right.offset;
    }
    if(m_ctrlSteerWheel.pressedDown){
        ++nPressed;
        offset.ry() += node.steerWheel.down.offset;
    }
    if(m_ctrlSteerWheel.pressedLeft){
        ++nPressed;
        offset.rx() -= node.steerWheel.left.offset;
    }
    // action
    //qDebug()<<nPressed<<"-"<<char(from->key())<<"-"<<from->type()<<"-"<<offset;
    if(nPressed == 0){ // release all
        int id = getTouchID(m_ctrlSteerWheel.touchKey);
        sendTouchUpEvent(id, node.steerWheel.centerPos + m_ctrlSteerWheel.lastOffset);
        detachTouchID(m_ctrlSteerWheel.touchKey);
    }else{
        int id;
        if(nPressed == 1 && flag){ // first press
            m_ctrlSteerWheel.touchKey = from->key();
            id = attachTouchID(m_ctrlSteerWheel.touchKey);
            sendTouchDownEvent(id, node.steerWheel.centerPos);
        }else{
            id = getTouchID(m_ctrlSteerWheel.touchKey);
        }
        sendTouchMoveEvent(id, node.steerWheel.centerPos + offset);
    }
    m_ctrlSteerWheel.lastOffset = offset;
    return;
}

// -------- key event --------

void InputConvertGame::processKeyClick(
        const QPointF& clickPos, bool clickTwice, bool switchMap, const QKeyEvent *from)
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

void InputConvertGame::processKeyDrag(const QPointF& startPos, QPointF endPos, const QKeyEvent* from)
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
    const KeyMap::KeyMapNode& node = m_keyMap.getKeyMapNodeMouse(from->button());
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
    if(m_ctrlMouseMove.touching){
        QPointF mousePos = from->localPos();
        mousePos.rx() /= m_showSize.width();
        mousePos.ry() /= m_showSize.height();
        QPointF offset = mousePos - m_ctrlMouseMove.startPosRel;
        //qDebug()<<from->localPos()<<" - "<<m_mouseMoveLastConverPos<<" - "<<offset<<" - "<<offset.manhattanLength();

        if(mousePos.x()<0.05 || mousePos.x()>0.95 || mousePos.y()<0.05 || mousePos.y()>0.95)
        {
            //qDebug()<<"reset";
            mouseMoveStopTouch();
            mouseMoveStartTouch(from);
        }
        offset /= m_keyMap.getMouseMoveMap().speedRatio;
        m_ctrlMouseMove.lastPosRel = m_ctrlMouseMove.startPosRel + offset;
        mouseMoveMovingTouch(m_ctrlMouseMove.lastPosRel);
    }else{
        m_ctrlMouseMove.touching = true;
        mouseMoveStartTouch(from);
        int left = from->globalX() - from->x();
        int top = from->globalY() - from->y();
        restrictMouse(left, left + m_showSize.width(), top, top+m_showSize.height());
    }
    return true;
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
    moveCursorTo(from, m_ctrlMouseMove.startPosPixel.toPoint());
    int id = attachTouchID(m_ctrlMouseMove.touchKey);
    sendTouchDownEvent(id, m_ctrlMouseMove.startPosRel);
    m_ctrlMouseMove.lastPosRel = m_ctrlMouseMove.startPosRel;
    m_ctrlMouseMove.touching = true;
}

void InputConvertGame::mouseMoveMovingTouch(const QPointF& target)
{
    sendTouchMoveEvent(getTouchID(m_ctrlMouseMove.touchKey), target);
}

void InputConvertGame::mouseMoveStopTouch()
{
    int id = getTouchID(m_ctrlMouseMove.touchKey);
    sendTouchUpEvent(id, m_ctrlMouseMove.lastPosRel);
    detachTouchID(m_ctrlMouseMove.touchKey);
    m_ctrlMouseMove.touching = false;
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
        if(m_ctrlMouseMove.touching)
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
