#include <QDebug>
#include <QCursor>
#include <QGuiApplication>

#include "inputconvertgame.h"

#define CURSOR_POS_CHECK 50

InputConvertGame::InputConvertGame(QObject* parent) : QObject(parent)
{

}

InputConvertGame::~InputConvertGame()
{

}

void InputConvertGame::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (m_gameMap) {
        updateSize(frameSize, showSize);

        // mouse move
        if (processMouseMove(from)) {
            return;
        }

        // mouse click
        if (processMouseClick(from)) {
            return;
        }
    } else {
        InputConvertNormal::mouseEvent(from, frameSize, showSize);
    }
}

void InputConvertGame::wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
    if (m_gameMap) {
        updateSize(frameSize, showSize);
    } else {
        InputConvertNormal::wheelEvent(from, frameSize, showSize);
    }
}

void InputConvertGame::keyEvent(const QKeyEvent *from, const QSize& frameSize, const QSize& showSize)
{
    switch (from->key()) {
    case Qt::Key_QuoteLeft:
        if (QEvent::KeyPress == from->type()) {
            if (!switchGameMap()) {
                m_needSwitchGameAgain = false;
            }
        }
        return;
    }

    if (m_needSwitchGameAgain) {
        updateSize(frameSize, showSize);
        // Qt::Key_Tab Qt::Key_M
        if (processKeyClick(from)) {
            return;
        }
    }

    if (m_gameMap) {
        updateSize(frameSize, showSize);
        if (!from || from->isAutoRepeat()) {
            return;
        }

        // steer wheel
        if (isSteerWheelKeys(from)) {
            processSteerWheel(from);
            return;
        }

        // key click
        if (processKeyClick(from)) {
            return;
        }
    } else {
        InputConvertNormal::keyEvent(from, frameSize, showSize);
    }
}

void InputConvertGame::updateSize(const QSize &frameSize, const QSize &showSize)
{
    m_frameSize = frameSize;
    m_showSize = showSize;
}

void InputConvertGame::sendTouchDownEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_DOWN);
}

void InputConvertGame::sendTouchMoveEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_MOVE);
}

void InputConvertGame::sendTouchUpEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_UP);
}

void InputConvertGame::sendTouchEvent(int id, QPointF pos, AndroidMotioneventAction action)
{
    if (0 > id || MULTI_TOUCH_MAX_NUM-1 < id) {
        return;
    }
    ControlEvent* controlEvent = new ControlEvent(ControlEvent::CET_TOUCH);
    if (!controlEvent) {
        return;
    }
    controlEvent->setTouchEventData(id, action, QRect(calcFrameAbsolutePos(pos).toPoint(), m_frameSize));
    sendControlEvent(controlEvent);
}

QPointF InputConvertGame::calcFrameAbsolutePos(QPointF relativePos)
{
    QPointF absolutePos;
    absolutePos.setX(m_frameSize.width() * relativePos.x());
    absolutePos.setY(m_frameSize.height() * relativePos.y());
    return absolutePos;
}

int InputConvertGame::attachTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (0 == multiTouchID[i]) {
            multiTouchID[i] = key;
            return i;
        }
    }
    return -1;
}

void InputConvertGame::detachTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == multiTouchID[i]) {
            multiTouchID[i] = 0;
            return;
        }
    }
}

int InputConvertGame::getTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == multiTouchID[i]) {
            return i;
        }
    }
    return -1;
}

bool InputConvertGame::isSteerWheelKeys(const QKeyEvent *from)
{
    for (int key : m_steerWheelKeys) {
        if (key == from->key()) {
            return true;
        }
    }
    return false;
}

void InputConvertGame::processSteerWheel(const QKeyEvent *from)
{    
    int keyPress1 = -1;
    int keyPress2 = -1;
    int keysNum = updateSteerWheelKeysPress(from, keyPress1, keyPress2);
    bool needMove = false;
    if (QEvent::KeyPress == from->type()) {
        if (1 == keysNum) {
            m_steerWheelFirstTouchKey = from->key();
            int id = attachTouchID(m_steerWheelFirstTouchKey);
            if (-1 == id) {
                return;
            }            
            sendTouchDownEvent(id, m_steerWheelPos);
            needMove = true;
        } else if (2 == keysNum) {
            needMove = true;
        }
    } else if (QEvent::KeyRelease == from->type()){
        if (0 == keysNum) {
            sendTouchUpEvent(getTouchID(m_steerWheelFirstTouchKey), m_steerWheelPos);
            detachTouchID(m_steerWheelFirstTouchKey);
            m_steerWheelFirstTouchKey = 0;
        } else if (1 == keysNum) {
            needMove = true;
        }
    }
    if (needMove) {
        steerWheelMove(keysNum, keyPress1, keyPress2);
    }
}

int InputConvertGame::updateSteerWheelKeysPress(const QKeyEvent *from, int& keyPress1, int& keyPress2)
{
    bool keyPress = false;
    if (QEvent::KeyPress == from->type()) {
        keyPress = true;
    } else if (QEvent::KeyRelease == from->type()) {
        keyPress = false;
    }
    if (from->key() == m_steerWheelKeys[SWD_UP]) {
        m_steerWheelKeysPress[SWD_UP] = keyPress;
    } else if (from->key() == m_steerWheelKeys[SWD_RIGHT]) {
        m_steerWheelKeysPress[SWD_RIGHT] = keyPress;
    } else if (from->key() == m_steerWheelKeys[SWD_DOWN]) {
        m_steerWheelKeysPress[SWD_DOWN] = keyPress;
    } else if (from->key() == m_steerWheelKeys[SWD_LEFT]) {
        m_steerWheelKeysPress[SWD_LEFT] = keyPress;
    }

    int count = 0;
    keyPress1 = -1;
    keyPress2 = -1;
    for (int i = 0; i < 4; i++) {
        if (true == m_steerWheelKeysPress[i]) {
            count++;

            if (-1 == keyPress1) {
                keyPress1 = i;
            } else if (-1 == keyPress2) {
                keyPress2 = i;
            }
        }
    }
    return count;
}

void InputConvertGame::steerWheelMove(int keysNum, int keyPress1, int keyPress2)
{
    if (1 != keysNum && 2 != keysNum) {
        return;
    }
    QPointF movePos = m_steerWheelPos;
    switch (keysNum) {
    case 2:
        if (keyPress2 == SWD_UP) {
            movePos.setY(movePos.y() - m_steerWheelOffset.top());
        } else if (keyPress2 == SWD_RIGHT) {
            movePos.setX(movePos.x() + m_steerWheelOffset.right());
        } else if (keyPress2 == SWD_DOWN) {
            movePos.setY(movePos.y() + m_steerWheelOffset.bottom());
        } else if (keyPress2 == SWD_LEFT) {
            movePos.setX(movePos.x() - m_steerWheelOffset.left());
        }
    case 1:
        if (keyPress1 == SWD_UP) {
            movePos.setY(movePos.y() - m_steerWheelOffset.top());
        } else if (keyPress1 == SWD_RIGHT) {
            movePos.setX(movePos.x() + m_steerWheelOffset.right());
        } else if (keyPress1 == SWD_DOWN) {
            movePos.setY(movePos.y() + m_steerWheelOffset.bottom());
        } else if (keyPress1 == SWD_LEFT) {
            movePos.setX(movePos.x() - m_steerWheelOffset.left());
        }
        break;
    }
    sendTouchMoveEvent(getTouchID(m_steerWheelFirstTouchKey), movePos);
}

bool InputConvertGame::processKeyClick(const QKeyEvent *from)
{
    QPointF clickPos;
    bool clickTwice = false;
    switch (from->key()) {
    case Qt::Key_Space: // 跳
        clickPos = QPointF(0.96f, 0.7f);
        break;
    case Qt::Key_M: // 地图
        if (QEvent::KeyRelease == from->type()) {
            m_needSwitchGameAgain = !m_needSwitchGameAgain;
            switchGameMap();
        }
        clickPos = QPointF(0.98f, 0.03f);
        break;
    case Qt::Key_Tab: // 背包
        if (QEvent::KeyRelease == from->type()) {
            m_needSwitchGameAgain = !m_needSwitchGameAgain;
            switchGameMap();
        }
        clickPos = QPointF(0.06f, 0.9f);        
        break;
    case Qt::Key_Z: // 趴
        clickPos = QPointF(0.95f, 0.9f);
        break;
    case Qt::Key_C: // 蹲
        clickPos = QPointF(0.86f, 0.92f);
        break;
    case Qt::Key_R: // 换弹
        clickPos = QPointF(0.795f, 0.93f);
        break;
    case Qt::Key_Alt: // 小眼睛
        clickPos = QPointF(0.8f, 0.31f);
        break;
    case Qt::Key_F: // 捡东西1
        clickPos = QPointF(0.7f, 0.34f);
        break;
    case Qt::Key_G: // 捡东西2
        clickPos = QPointF(0.7f, 0.44f);
        break;
    case Qt::Key_H: // 捡东西3
        clickPos = QPointF(0.7f, 0.54f);
        break;
    case Qt::Key_1: // 换枪1
        clickPos = QPointF(0.45f, 0.9f);
        break;
    case Qt::Key_2: // 换枪2
        clickPos = QPointF(0.55f, 0.9f);
        break;
    case Qt::Key_3: // 手雷
        clickPos = QPointF(0.67f, 0.92f);
        break;
    case Qt::Key_4: // 快速打药
        clickPos = QPointF(0.33f, 0.95f);
        break;
    case Qt::Key_5: // 下车
        clickPos = QPointF(0.92f, 0.4f);
        break;
    case Qt::Key_6: // 救人
        clickPos = QPointF(0.49f, 0.63f);
        break;
    case Qt::Key_Shift: // 车加速
        clickPos = QPointF(0.82f, 0.8f);
        break;
    case Qt::Key_X: // 开关门
        clickPos = QPointF(0.7f, 0.7f);
        break;
    case Qt::Key_T: // 舔包
        clickPos = QPointF(0.72f, 0.26f);
        break;
    case Qt::Key_Q: // 左探头
        clickTwice = true;
        clickPos = QPointF(0.12f, 0.35f);
        break;
    case Qt::Key_E: // 右探头
        clickTwice = true;
        clickPos = QPointF(0.2, 0.35f);
        break;
    default:
        return false;
        break;
    }

    if (QEvent::KeyPress == from->type()) {
        int id = attachTouchID(from->key());
        sendTouchDownEvent(id, clickPos);
        if (clickTwice) {
            sendTouchUpEvent(getTouchID(from->key()), clickPos);
            detachTouchID(from->key());
        }
    } else if (QEvent::KeyRelease == from->type()) {
        if (clickTwice) {
            int id = attachTouchID(from->key());
            sendTouchDownEvent(id, clickPos);
        }
        sendTouchUpEvent(getTouchID(from->key()), clickPos);
        detachTouchID(from->key());
    }
    return true;
}

bool InputConvertGame::processMouseClick(const QMouseEvent *from)
{
    QPointF clickPos;
    if (Qt::LeftButton == from->button()) {
        clickPos = QPointF(0.86f, 0.72f);
    } else if (Qt::RightButton == from->button()){
        clickPos = QPointF(0.96f, 0.52f);
    } else {
        return false;
    }

    if (QEvent::MouseButtonPress == from->type() || QEvent::MouseButtonDblClick == from->type()) {
        int id = attachTouchID(from->button());
        sendTouchDownEvent(id, clickPos);
    } else if (QEvent::MouseButtonRelease == from->type()) {
        sendTouchUpEvent(getTouchID(from->button()), clickPos);
        detachTouchID(from->button());
    } else {
        return false;
    }
    return true;
}

bool InputConvertGame::processMouseMove(const QMouseEvent *from)
{
    if (QEvent::MouseMove != from->type()) {
        return false;
    }    

    if (checkCursorPos(from)) {
        m_mouseMoveLastPos = QPointF(0.0f, 0.0f);
        return true;
    }

    if (!m_mouseMoveLastPos.isNull()) {
        QPointF distance = from->localPos() - m_mouseMoveLastPos;
        distance /= 10;

        mouseMoveStartTouch(from);
        startMouseMoveTimer();

        m_mouseMoveLastConverPos.setX(m_mouseMoveLastConverPos.x() + distance.x() / m_showSize.width());
        m_mouseMoveLastConverPos.setY(m_mouseMoveLastConverPos.y() + distance.y() / m_showSize.height());

        if (m_mouseMoveLastConverPos.x() < 0.1
                || m_mouseMoveLastConverPos.x() > 0.9
                || m_mouseMoveLastConverPos.y() < 0.1
                || m_mouseMoveLastConverPos.y() > 0.9) {
            mouseMoveStopTouch();
            mouseMoveStartTouch(from);
        }

        sendTouchMoveEvent(getTouchID(Qt::ExtraButton24), m_mouseMoveLastConverPos);
    }
    m_mouseMoveLastPos = from->localPos();
    return true;
}

void InputConvertGame::moveCursorToStart(const QMouseEvent *from)
{
    QPoint localPos = QPoint(m_showSize.width()*m_mouseMoveStartPos.x(), m_showSize.height()*m_mouseMoveStartPos.y());
    QPoint posOffset = from->localPos().toPoint() - localPos;
    QPoint globalPos = from->globalPos();

    globalPos -= posOffset;
    QCursor::setPos(globalPos);
}

void InputConvertGame::moveCursorTo(const QMouseEvent *from, const QPoint &pos)
{
    QPoint posOffset = from->localPos().toPoint() - pos;
    QPoint globalPos = from->globalPos();

    globalPos -= posOffset;
    QCursor::setPos(globalPos);
}

void InputConvertGame::startMouseMoveTimer()
{
    stopMouseMoveTimer();
    m_mouseMoveTimer = startTimer(1000);
}

void InputConvertGame::stopMouseMoveTimer()
{
    if (0 != m_mouseMoveTimer) {
        killTimer(m_mouseMoveTimer);
        m_mouseMoveTimer = 0;
    }
}

void InputConvertGame::mouseMoveStartTouch(const QMouseEvent* from)
{
    if (!m_mouseMovePress) {
        //moveCursorToStart(from);
        int id = attachTouchID(Qt::ExtraButton24);
        sendTouchDownEvent(id, m_mouseMoveStartPos);
        m_mouseMoveLastConverPos = m_mouseMoveStartPos;
        m_mouseMovePress = true;
    }
}

void InputConvertGame::mouseMoveStopTouch()
{
    if (m_mouseMovePress) {
        sendTouchUpEvent(getTouchID(Qt::ExtraButton24), m_mouseMoveLastConverPos);
        detachTouchID(Qt::ExtraButton24);
        m_mouseMovePress = false;
    }
}

bool InputConvertGame::switchGameMap()
{
    m_gameMap = !m_gameMap;    
    emit grabCursor(m_gameMap);
    if (m_gameMap) {
        QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
    } else {
        mouseMoveStopTouch();        
        QGuiApplication::restoreOverrideCursor();
    }
    return m_gameMap;
}

bool InputConvertGame::checkCursorPos(const QMouseEvent *from)
{
    bool moveCursor = false;
    QPoint pos = from->pos();
    if (pos.x() < CURSOR_POS_CHECK) {
        pos.setX(m_showSize.width() - CURSOR_POS_CHECK);
        moveCursor = true;
    } else if (pos.x() > m_showSize.width() - CURSOR_POS_CHECK) {
        pos.setX(CURSOR_POS_CHECK);
        moveCursor = true;
    } else if (pos.y() < CURSOR_POS_CHECK) {
        pos.setY(m_showSize.height() - CURSOR_POS_CHECK);
        moveCursor = true;
    } else if (pos.y() > m_showSize.height() - CURSOR_POS_CHECK) {
        pos.setY(CURSOR_POS_CHECK);
        moveCursor = true;
    }

    if (moveCursor) {
        moveCursorTo(from, pos);
    }

    return moveCursor;
}

void InputConvertGame::timerEvent(QTimerEvent *event)
{
    if (m_mouseMoveTimer == event->timerId()) {
        stopMouseMoveTimer();
        mouseMoveStopTouch();
    }
}
