#ifndef INPUTCONVERT_H
#define INPUTCONVERT_H

#include "inputconvertbase.h"

class InputConvertNormal : public InputConvertBase
{
public:
    InputConvertNormal();
    virtual ~InputConvertNormal();

    void mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize);
    void wheelEvent(const QWheelEvent* from, const QSize& frameSize, const QSize& showSize);
    void keyEvent(const QKeyEvent* from, const QSize& frameSize, const QSize& showSize);

private:
    AndroidMotioneventButtons convertMouseButtons(Qt::MouseButtons buttonState);
    AndroidKeycode convertKeyCode(int key, Qt::KeyboardModifiers modifiers);
    AndroidMetastate convertMetastate(Qt::KeyboardModifiers modifiers);
};

#endif // INPUTCONVERT_H
