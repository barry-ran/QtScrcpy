#ifndef INPUTCONVERT_H
#define INPUTCONVERT_H

#include <QMouseEvent>
#include <QWheelEvent>

#include "controlevent.h"

class InputConvert
{
public:
    InputConvert();

    // the frame size may be different from the real device size, so we need the size
    // to which the absolute position apply, to scale it accordingly
    static ControlEvent* mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize);
    static ControlEvent* wheelEvent(const QWheelEvent* from, const QSize& frameSize, const QSize& showSize);

private:
    static AndroidMotioneventButtons convertMouseButtons(Qt::MouseButtons buttonState);
};

#endif // INPUTCONVERT_H
