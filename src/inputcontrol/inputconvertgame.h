#ifndef INPUTCONVERTGAME_H
#define INPUTCONVERTGAME_H

#include "inputconvertbase.h"

class InputConvertGame : public InputConvertBase
{
public:
    InputConvertGame();
    virtual ~InputConvertGame();

    void mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize);
    void wheelEvent(const QWheelEvent* from, const QSize& frameSize, const QSize& showSize);
    void keyEvent(const QKeyEvent* from, const QSize& frameSize, const QSize& showSize);
};

#endif // INPUTCONVERTGAME_H
