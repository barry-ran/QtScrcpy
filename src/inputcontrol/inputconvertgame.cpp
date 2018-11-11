#include "inputconvertgame.h"

InputConvertGame::InputConvertGame()
{

}

InputConvertGame::~InputConvertGame()
{

}

void InputConvertGame::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    updateSize(frameSize, showSize);
}

void InputConvertGame::wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
    updateSize(frameSize, showSize);
}

void InputConvertGame::keyEvent(const QKeyEvent *from, const QSize& frameSize, const QSize& showSize)
{
    updateSize(frameSize, showSize);
    if (!from || from->isAutoRepeat()) {
        return;
    }

    if (isSteerWheelKeys(from)) {
        processSteerWheel(from);
        return;
    }

    AndroidMotioneventAction action;
    // pos
    QPointF pos;
    // id
    int id = 0;

    if (from->key() == Qt::Key_A) {
        id = 0;
        pos.setX(showSize.width() * 0.25f);
        pos.setY(showSize.height() * 0.25f);
    } else if (from->key() == Qt::Key_S) {
        id = 1;
        pos.setX(showSize.width() * 0.35f);
        pos.setY(showSize.height() * 0.35f);
    } else {
        return;
    }

    // action
    switch (from->type()) {
    case QEvent::KeyPress:
        action = AMOTION_EVENT_ACTION_DOWN;
        break;
    case QEvent::KeyRelease:
        action = AMOTION_EVENT_ACTION_UP;
        break;
    default:
        return;
    }    

    // convert pos
    pos.setX(pos.x() * frameSize.width() / showSize.width());
    pos.setY(pos.y() * frameSize.height() / showSize.height());

    // set data
    ControlEvent* controlEvent = new ControlEvent(ControlEvent::CET_TOUCH);
    if (!controlEvent) {
        return;
    }    
    controlEvent->setTouchEventData(id, action, QRect(pos.toPoint(), frameSize));
    sendControlEvent(controlEvent);

    if (QEvent::KeyPress == from->type() && from->key() == Qt::Key_S) {
        // set data
        ControlEvent* controlEvent2 = new ControlEvent(ControlEvent::CET_TOUCH);
        if (!controlEvent2) {
            return;
        }
        pos.setX(pos.x() + 50);
        pos.setY(pos.y() + 50);
        controlEvent2->setTouchEventData(id, AMOTION_EVENT_ACTION_MOVE, QRect(pos.toPoint(), frameSize));
        sendControlEvent(controlEvent2);
    }

    return;
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
    ControlEvent* controlEvent = new ControlEvent(ControlEvent::CET_TOUCH);
    if (!controlEvent) {
        return;
    }
    controlEvent->setTouchEventData(id, action, QRect(calcAbsolutePos(pos).toPoint(), m_frameSize));
    sendControlEvent(controlEvent);
}

QPointF InputConvertGame::calcAbsolutePos(QPointF relativePos)
{
    QPointF absolutePos;
    absolutePos.setX(m_showSize.width() * relativePos.x());
    absolutePos.setY(m_showSize.height() * relativePos.y());
    // convert pos
    absolutePos.setX(absolutePos.x() * m_frameSize.width() / m_showSize.width());
    absolutePos.setY(absolutePos.y() * m_frameSize.height() / m_showSize.height());
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

bool InputConvertGame::isSteerWheelKeys(const QKeyEvent *from)
{
    for (int key : m_steerWheelKeys) {
        if (key == from->key()) {
            return true;
        }
    }
    return false;
}
#include <QDebug>
void InputConvertGame::processSteerWheel(const QKeyEvent *from)
{    
    QPointF movePos = m_steerWheelPos;
    int moveKey = 0;
    if (QEvent::KeyPress == from->type()) {
        m_steerWheelKeysNum++;        
        if (1 == m_steerWheelKeysNum) {
            int id = attachTouchID(from->key());
            if (-1 == id) {
                return;
            }
            m_steerWheelFirstTouchKey = from->key();
            sendTouchDownEvent(id, m_steerWheelPos);
            moveKey = from->key();
        } else if (2 == m_steerWheelKeysNum) {
            moveKey = from->key();
        }

    } else if (QEvent::KeyRelease == from->type()){
        m_steerWheelKeysNum--;
        if (0 == m_steerWheelKeysNum) {
            sendTouchUpEvent(getTouchID(m_steerWheelFirstTouchKey), m_steerWheelPos);
            detachTouchID(from->key());
            m_steerWheelFirstTouchKey = 0;
            return;
        } else if (1 == m_steerWheelKeysNum) {
            moveKey = m_steerWheelFirstTouchKey;
        }
    }

    if (moveKey == m_steerWheelKeys[SWD_UP]) {
        movePos.setY(movePos.y() - m_steerWheelOffset);
    } else if (moveKey == m_steerWheelKeys[SWD_RIGHT]) {
        movePos.setX(movePos.x() + m_steerWheelOffset);
    } else if (moveKey == m_steerWheelKeys[SWD_DOWN]) {
        movePos.setY(movePos.y() + m_steerWheelOffset);
    } else if (moveKey == m_steerWheelKeys[SWD_LEFT]) {
        movePos.setX(movePos.x() - m_steerWheelOffset);
    }
    if (0 != moveKey) {
        qDebug() << "move pos" << movePos;
        sendTouchMoveEvent(getTouchID(m_steerWheelFirstTouchKey), movePos);
    }
}
