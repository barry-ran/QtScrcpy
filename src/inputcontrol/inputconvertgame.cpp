#include "inputconvertgame.h"

InputConvertGame::InputConvertGame()
{

}

InputConvertGame::~InputConvertGame()
{

}

void InputConvertGame::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
}

void InputConvertGame::wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
}

void InputConvertGame::keyEvent(const QKeyEvent *from, const QSize& frameSize, const QSize& showSize)
{
    if (!from || from->isAutoRepeat()) {
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
/*
    if (QEvent::KeyPress == from->type()) {
        ControlEvent* controlEvent2 = new ControlEvent(ControlEvent::CET_MOUSE);
        if (!controlEvent2) {
            return;
        }
        pos.setY(pos.y() - 50);
        action &= AMOTION_EVENT_ACTION_POINTER_INDEX_MASK;
        action |= AMOTION_EVENT_ACTION_MOVE;
        controlEvent2->setMouseEventData((AndroidMotioneventAction)action, AMOTION_EVENT_BUTTON_PRIMARY, QRect(pos.toPoint(), frameSize));
        sendControlEvent(controlEvent2);
    }
    */
}
