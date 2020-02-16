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

    //virtual void
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
    void processSteerWheel(const KeyMap::KeyMapNode &node,
                           int keyButtonID, bool isPress);

    // click
    void processClick(const QPointF& clickPos, bool clickTwice, bool switchMap,
                      int keyButtonID, bool isPress, bool isRelease);

    // drag
    void processDrag(const QPointF& startPos, QPointF endPos,
                     int keyButtonID, bool isPress, bool isRelease);

    // free look
    void processFreeLook(const QPointF& startPos, const double speedRatio,
                         bool isPress, bool isRelease);

    // mouse
    void processMouseMove(const QMouseEvent* from);
    void moveCursorTo(const QMouseEvent* from, const QPoint& localPosPixel);
    void mouseMoveStartTouch(const QMouseEvent* from);
    void mouseMoveStopTouch();
    void startMouseMoveTimer();
    void stopMouseMoveTimer();

    bool switchGameMap();
    bool checkCursorPos(const QMouseEvent *from);

protected:
    void timerEvent(QTimerEvent *event);

private:
    QSize m_frameSize;
    QSize m_showSize;
    bool m_gameMap = false;
    bool m_needSwitchGameAgain = false;
    int m_multiTouchID[MULTI_TOUCH_MAX_NUM] = { 0 };
    KeyMap m_keyMap;

    // steer wheel
    struct {
        // the first key pressed
        int touchKey = Qt::Key_unknown;
        bool pressedUp = false;
        bool pressedDown = false;
        bool pressedLeft = false;
        bool pressedRight = false;
        // for last up
        QPointF lastOffset;
    } m_ctrlSteerWheel;

    // mouse move
    struct {
        QPointF lastConverPos;
        QPointF lastPos = {0.0, 0.0};
        bool touching = false;
        int timer = 0;
        struct {
            QPointF startPos;
            double speedRatio;
        } detail;
        bool valid;
    } m_ctrlMouseMove;
};

#endif // INPUTCONVERTGAME_H
