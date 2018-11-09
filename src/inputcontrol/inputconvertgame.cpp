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

    // action
    AndroidMotioneventAction action;
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

    // pos
    QPointF pos;
    pos.setX(showSize.width() * 0.25f);
    pos.setY(showSize.height() * 0.5f);

    // convert pos
    pos.setX(pos.x() * frameSize.width() / showSize.width());
    pos.setY(pos.y() * frameSize.height() / showSize.height());

    // set data
    ControlEvent* controlEvent = new ControlEvent(ControlEvent::CET_MOUSE);
    if (!controlEvent) {
        return;
    }
    controlEvent->setMouseEventData(action, AMOTION_EVENT_BUTTON_PRIMARY, QRect(pos.toPoint(), frameSize));
    sendControlEvent(controlEvent);

    if (QEvent::KeyPress == from->type()) {
        ControlEvent* controlEvent2 = new ControlEvent(ControlEvent::CET_MOUSE);
        if (!controlEvent2) {
            return;
        }
        pos.setY(pos.y() - 50);
        controlEvent2->setMouseEventData(AMOTION_EVENT_ACTION_MOVE, AMOTION_EVENT_BUTTON_PRIMARY, QRect(pos.toPoint(), frameSize));
        sendControlEvent(controlEvent2);
    }
}
