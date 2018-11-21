#ifndef INPUTCONVERTGAME_H
#define INPUTCONVERTGAME_H

#include <QPointF>
#include "inputconvertnormal.h"

#define MULTI_TOUCH_MAX_NUM 10
class InputConvertGame : public QObject, public InputConvertNormal
{
    Q_OBJECT
public:
    InputConvertGame(QObject* parent = Q_NULLPTR);
    virtual ~InputConvertGame();

    virtual void mouseEvent(const QMouseEvent* from, const QSize& frameSize, const QSize& showSize);
    virtual void wheelEvent(const QWheelEvent* from, const QSize& frameSize, const QSize& showSize);
    virtual void keyEvent(const QKeyEvent* from, const QSize& frameSize, const QSize& showSize);

signals:
    void grabCursor(bool grab);

protected:
    void updateSize(const QSize& frameSize, const QSize& showSize);
    void sendTouchDownEvent(int id, QPointF pos);
    void sendTouchMoveEvent(int id, QPointF pos);
    void sendTouchUpEvent(int id, QPointF pos);
    void sendTouchEvent(int id, QPointF pos, AndroidMotioneventAction action);
    QPointF calcFrameAbsolutePos(QPointF relativePos);

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
    void moveCursorToStart(const QMouseEvent* from);
    void moveCursorTo(const QMouseEvent* from, const QPoint& pos);
    void startMouseMoveTimer();
    void stopMouseMoveTimer();
    void mouseMoveStartTouch(const QMouseEvent* from);
    void mouseMoveStopTouch();

    bool switchGameMap();
    bool checkCursorPos(const QMouseEvent* from);

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
    bool m_gameMap = false;

    int multiTouchID[MULTI_TOUCH_MAX_NUM] = { 0 };

    QPointF m_steerWheelPos = {0.16f, 0.75f};    
    QRectF m_steerWheelOffset = {QPointF(0.1f, 0.27f), QPointF(0.1f, 0.2f)};
    // order by SteerWheelDirection(up right down left)
    int m_steerWheelKeys[4] = {Qt::Key_W, Qt::Key_D, Qt::Key_S, Qt::Key_A};
    bool m_steerWheelKeysPress[4] = { false };
    int m_steerWheelKeysNum = 0;
    int m_steerWheelFirstTouchKey = 0;

    // mouse move
    QPointF m_mouseMoveStartPos = {0.57f, 0.26f};    
    QPointF m_mouseMoveLastConverPos = m_mouseMoveStartPos;
    QPointF m_mouseMoveLastPos = {0.0f, 0.0f};
    bool m_mouseMovePress = false;
    int m_mouseMoveTimer = 0;

    bool m_needSwitchGameAgain = false;
};

#endif // INPUTCONVERTGAME_H
