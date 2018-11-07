#include "inputconvert.h"

InputConvert::InputConvert()
{

}

ControlEvent* InputConvert::mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize)
{
    if (!from) {
        return Q_NULLPTR;
    }

    // action
    AndroidMotioneventAction action;
    switch (from->type()) {
    case QEvent::MouseButtonPress:
        action = AMOTION_EVENT_ACTION_DOWN;
        break;
    case QEvent::MouseButtonRelease:
        action = AMOTION_EVENT_ACTION_UP;
        break;
    case QEvent::MouseMove:
        action = AMOTION_EVENT_ACTION_MOVE;
        break;
    default:
        return Q_NULLPTR;
    }

    // pos
    QPointF pos = from->localPos();
    // convert pos
    pos.setX(pos.x() * frameSize.width() / showSize.width());
    pos.setY(pos.y() * frameSize.height() / showSize.height());    

    // set data
    ControlEvent* controlEvent = new ControlEvent(ControlEvent::CET_MOUSE);
    if (!controlEvent) {
        return Q_NULLPTR;
    }
    controlEvent->setMouseEventData(action, convertMouseButtons(from->buttons()), QRect(pos.toPoint(), frameSize));
    return controlEvent;
}

ControlEvent *InputConvert::wheelEvent(const QWheelEvent *from, const QSize& frameSize, const QSize& showSize)
{
    if (!from) {
        return Q_NULLPTR;
    }    

    // delta
    qint32 hScroll = 0;
    qint32 vScroll = 0;
    switch (from->orientation()) {
    case Qt::Horizontal:
        hScroll = from->delta();
        break;
    case Qt::Vertical:
        vScroll = from->delta();
        break;
    }

    // pos
    QPointF pos = from->posF();
    // convert pos
    pos.setX(pos.x() * frameSize.width() / showSize.width());
    pos.setY(pos.y() * frameSize.height() / showSize.height());

    // set data
    ControlEvent* controlEvent = new ControlEvent(ControlEvent::CET_SCROLL);
    if (!controlEvent) {
        return Q_NULLPTR;
    }
    controlEvent->setScrollEventData(QRect(pos.toPoint(), frameSize), hScroll, vScroll);
    return controlEvent;
}

AndroidMotioneventButtons InputConvert::convertMouseButtons(Qt::MouseButtons buttonState)
{
    quint32 buttons = 0;
    if (buttonState & Qt::LeftButton) {
        buttons |= AMOTION_EVENT_BUTTON_PRIMARY;
    }
    if (buttonState & Qt::RightButton) {
        buttons |= AMOTION_EVENT_BUTTON_SECONDARY;
    }
    if (buttonState & Qt::MidButton) {
        buttons |= AMOTION_EVENT_BUTTON_TERTIARY;
    }
    if (buttonState & Qt::XButton1) {
        buttons |= AMOTION_EVENT_BUTTON_BACK;
    }
    if (buttonState & Qt::XButton2) {
        buttons |= AMOTION_EVENT_BUTTON_FORWARD;
    }
    return (AndroidMotioneventButtons)buttons;
}
