#ifndef INPUTCONVERTGAME_H
#define INPUTCONVERTGAME_H

#include <QPointF>

#include "inputconvertnormal.h"
#include "keymap.h"

#define MULTI_TOUCH_MAX_NUM 10
class InputConvertGame : public InputConvertNormal
{
    Q_OBJECT
public:
    InputConvertGame(Controller* controller);
    virtual ~InputConvertGame();

    virtual void mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize);
    virtual void wheelEvent(const QWheelEvent* from, const QSize& frameSize, const QSize& showSize);
    virtual void keyEvent(const QKeyEvent* from, const QSize& frameSize, const QSize& showSize);

    void loadKeyMap(const QString& json);

protected:
    void updateSize(const QSize& frameSize, const QSize& showSize);
    void sendTouchDownEvent(int id, QPointF pos);
    void sendTouchMoveEvent(int id, QPointF pos);
    void sendTouchUpEvent(int id, QPointF pos);
    void sendTouchEvent(int id, QPointF pos, AndroidMotioneventAction action);
    QPointF calcFrameAbsolutePos(QPointF relativePos);
    QPointF calcScreenAbsolutePos(QPointF relativePos);

    // multi touch id
    int attachTouchID(int key);
    void detachTouchID(int key);
    int getTouchID(int key);

    // steer wheel
    void processSteerWheel(const KeyMap::KeyMapNode &node, const QKeyEvent* from);

    // click
    void processKeyClick(const QPointF& clickPos, bool clickTwice, bool switchMap, const QKeyEvent* from);

    // drag
    void processKeyDrag(const QPointF& startPos, QPointF endPos, const QKeyEvent* from);

    // mouse
    bool processMouseClick(const QMouseEvent* from);
    bool processMouseMove(const QMouseEvent* from);
    void moveCursorTo(const QMouseEvent* from, const QPoint& localPosPixel);
    void mouseMoveStartTouch(const QMouseEvent* from);
    void mouseMoveMovingTouch(const QPointF& target);
    void mouseMoveStopTouch();

    void startMouseMoveTimer();
    void stopMouseMoveTimer();

    bool switchGameMap();

protected:
    void timerEvent(QTimerEvent *event);

private:
    QSize m_frameSize;
    QSize m_showSize;
    bool m_gameMap = false;

    int multiTouchID[MULTI_TOUCH_MAX_NUM] = { 0 };

    // steer wheel
    struct{
        bool valid = false;
        bool touching = false;
        int touchKey = Qt::Key_unknown; // the first key pressed
        int nKeyPressed = 0;
        bool pressedUp = false, pressedDown = false;
        bool pressedLeft = false, pressedRight = false;
        QPointF centerPos;
        QPointF lastOffset;
    } m_ctrlSteerWheel;

    // mouse move
    struct{
        bool valid = false;
        bool touching = false;
        const int touchKey = Qt::ExtraButton24;
        QPointF startPosRel; // in [0, 1)
        QPointF startPosPixel; // in [0, size)
        QPointF lastPosRel;
        //QPointF lastPosPixel;
    } m_ctrlMouseMove;

    int m_mouseMoveTimer = 0;

    bool m_needSwitchGameAgain = false;

    KeyMap m_keyMap;
};

#endif // INPUTCONVERTGAME_H
