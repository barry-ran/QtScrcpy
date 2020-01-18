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

    // multi touch id
    int attachTouchID(int key);
    void detachTouchID(int key);
    int getTouchID(int key);

    // steer wheel
    void processSteerWheel(KeyMap::KeyMapNode &node, const QKeyEvent* from);
    int updateSteerWheelKeysPress(KeyMap::KeyMapNode &node, const QKeyEvent* from, int& keyPress1, int& keyPress2);
    void steerWheelMove(KeyMap::KeyMapNode &node, int keysNum, int keyPress1, int keyPress2);

    // click
    void processKeyClick(QPointF clickPos, bool clickTwice, bool switchMap, const QKeyEvent* from);

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

    // mouse move    
    QPointF m_mouseMoveLastConverPos;
    QPointF m_mouseMoveLastPos = {0.0f, 0.0f};
    bool m_mouseMovePress = false;
    int m_mouseMoveTimer = 0;

    bool m_needSwitchGameAgain = false;

    KeyMap m_keyMap;
};

#endif // INPUTCONVERTGAME_H
