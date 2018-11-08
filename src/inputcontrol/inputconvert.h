#ifndef INPUTCONVERT_H
#define INPUTCONVERT_H

#include "inputconvertbase.h"

class InputConvert : public InputConvertBase
{
public:
    InputConvert();
    virtual ~InputConvert();

    ControlEvent* mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize);
    ControlEvent* wheelEvent(const QWheelEvent* from, const QSize& frameSize, const QSize& showSize);
    ControlEvent* keyEvent(const QKeyEvent* from);

private:
    AndroidMotioneventButtons convertMouseButtons(Qt::MouseButtons buttonState);
    AndroidKeycode convertKeyCode(int key, Qt::KeyboardModifiers modifiers);
    AndroidMetastate convertMetastate(Qt::KeyboardModifiers modifiers);
};

#endif // INPUTCONVERT_H
