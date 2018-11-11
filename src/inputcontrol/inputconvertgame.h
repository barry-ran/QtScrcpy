#ifndef INPUTCONVERTGAME_H
#define INPUTCONVERTGAME_H

#include <QPointF>
#include "inputconvertbase.h"

#define MULTI_TOUCH_MAX_NUM 10
class InputConvertGame : public InputConvertBase
{
public:
    InputConvertGame();
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
    float m_steerWheelOffset = 0.1f;
    // order by SteerWheelDirection(up right down left)
    int m_steerWheelKeys[4] = {Qt::Key_W, Qt::Key_D, Qt::Key_S, Qt::Key_A};
    int m_steerWheelKeysNum = 0;
    int m_steerWheelFirstTouchKey = 0;
};

#endif // INPUTCONVERTGAME_H
