#ifndef INPUTCONVERTBASE_H
#define INPUTCONVERTBASE_H

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include "controlmsg.h"
#include "controller.h"

class InputConvertBase
{
public:
    InputConvertBase();
    virtual ~InputConvertBase();

    // the frame size may be different from the real device size, so we need the size
    // to which the absolute position apply, to scale it accordingly
    virtual void mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize) = 0;
    virtual void wheelEvent(const QWheelEvent* from, const QSize& frameSize, const QSize& showSize) = 0;
    virtual void keyEvent(const QKeyEvent* from, const QSize& frameSize, const QSize& showSize) = 0;

    void setControlSocket(QTcpSocket* controlSocket);
    void sendControlMsg(ControlMsg* msg);

private:
    Controller m_controller;
};

#endif // INPUTCONVERTBASE_H
