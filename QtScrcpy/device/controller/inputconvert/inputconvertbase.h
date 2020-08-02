#ifndef INPUTCONVERTBASE_H
#define INPUTCONVERTBASE_H

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QWheelEvent>

#include "controlmsg.h"

class Controller;
class InputConvertBase : public QObject
{
    Q_OBJECT
public:
    InputConvertBase(Controller *controller);
    virtual ~InputConvertBase();

    // the frame size may be different from the real device size, so we need the size
    // to which the absolute position apply, to scale it accordingly
    virtual void mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize) = 0;
    virtual void wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize) = 0;
    virtual void keyEvent(const QKeyEvent *from, const QSize &frameSize, const QSize &showSize) = 0;
    virtual bool isCurrentCustomKeymap()
    {
        return false;
    };

signals:
    void grabCursor(bool grab);

protected:
    void sendControlMsg(ControlMsg *msg);

private:
    QPointer<Controller> m_controller;
};

#endif // INPUTCONVERTBASE_H
