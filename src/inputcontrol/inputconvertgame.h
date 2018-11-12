#ifndef INPUTCONVERTGAME_H
#define INPUTCONVERTGAME_H

#include <QPointF>
#include "inputconvertbase.h"

#define MULTI_TOUCH_MAX_NUM 10
class InputConvertGame : public QObject, public InputConvertBase
{
    Q_OBJECT
public:
    InputConvertGame(QObject* parent = Q_NULLPTR);
    virtual ~InputConvertGame();

    void mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize);
    void wheelEvent(const QWheelEvent* from, const QSize& frameSize, const QSize& showSize);
    void keyEvent(const QKeyEvent* from, const QSize& frameSize, const QSize& showSize);

protected:
    void updateSize(const QSize& frameSize, const QSize& showSize);
    void sendTouchDownEvent(int id, QPointF pos);
    void sendTouchMoveEvent(int id, QPointF pos);
    void sendTouchUpEvent(int id, QPointF pos);
    void sendTouchEvent(int id, QPointF pos, AndroidMotioneventAction action);
    QPointF calcAbsolutePos(QPointF relativePos);

    // multi touch id
    int attachTouchID(int key);
    void detachTouchID(int key);
    int getTouchID(int key);

    // steer wheel
    bool isSteerWheelKeys(const QKeyEvent* from);
    void processSteerWheel(const QKeyEvent* from);
    int updateSteerWheelKeysPress(const QKeyEvent* from, int& keyPress1, int& keyPress2);
    void steerWheelMove(int keysNum, int keyPress1, int keyPress2);

    // click
    bool processKeyClick(const QKeyEvent* from);

    // mouse
    bool processMouseClick(const QMouseEvent* from);
    bool processMouseMove(const QMouseEvent* from);
    void startMouseMoveTimer();
    void stopMouseMoveTimer();
    void mouseMoveStartTouch();
    void mouseMoveStopTouch();

protected:
    void timerEvent(QTimerEvent *event);

private:
    enum SteerWheelDirection {
        SWD_UP = 0,
        SWD_RIGHT,
        SWD_DOWN,
        SWD_LEFT,
    };

private:
    QSize m_frameSize;
    QSize m_showSize;

    int multiTouchID[MULTI_TOUCH_MAX_NUM] = { 0 };

    QPointF m_steerWheelPos = {0.16f, 0.75f};
    QPointF m_steerWheelOffset = {0.1f, 0.2f};
    // order by SteerWheelDirection(up right down left)
    int m_steerWheelKeys[4] = {Qt::Key_W, Qt::Key_D, Qt::Key_S, Qt::Key_A};
    bool m_steerWheelKeysPress[4] = { false };
    int m_steerWheelKeysNum = 0;
    int m_steerWheelFirstTouchKey = 0;

    // mouse move
    QPointF m_mouseMoveStartPos = {0.57f, 0.26f};
    bool m_mouseMovePress = false;
    int m_mouseMoveTimer = 0;
};

#endif // INPUTCONVERTGAME_H
