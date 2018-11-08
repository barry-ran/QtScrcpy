#ifndef INPUTCONVERTBASE_H
#define INPUTCONVERTBASE_H

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include "controlevent.h"

class InputConvertBase
{
public:
    InputConvertBase();
    virtual ~InputConvertBase();

    // the frame size may be different from the real device size, so we need the size
    // to which the absolute position apply, to scale it accordingly
    virtual ControlEvent* mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize) = 0;
    virtual ControlEvent* wheelEvent(const QWheelEvent* from, const QSize& frameSize, const QSize& showSize) = 0;
    virtual ControlEvent* keyEvent(const QKeyEvent* from) = 0;
};

#endif // INPUTCONVERTBASE_H
