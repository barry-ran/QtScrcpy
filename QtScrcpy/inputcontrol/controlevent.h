#ifndef CONTROLEVENT_H
#define CONTROLEVENT_H

#include <QRect>
#include <QString>
#include <QBuffer>

#include "qscrcpyevent.h"
#include "input.h"
#include "keycodes.h"

#define CONTROL_EVENT_COMMAND_BACK_OR_SCREEN_ON 0

#define TEXT_MAX_CHARACTER_LENGTH 300
// ControlEvent
class ControlEvent : public QScrcpyEvent
{
public:
    enum ControlEventType {
        CET_KEYCODE,
        CET_TEXT,
        CET_MOUSE,        
        CET_SCROLL,
        CET_COMMAND,
        CET_TOUCH,
    };   

    ControlEvent(ControlEventType controlEventType);

    void setKeycodeEventData(AndroidKeyeventAction action, AndroidKeycode keycode, AndroidMetastate metastate);
    void setTextEventData(QString text);
    void setMouseEventData(AndroidMotioneventAction action, AndroidMotioneventButtons buttons, QRect position);
    // id 代表一个触摸点，最多支持10个触摸点[0,9]
    // action 只能是AMOTION_EVENT_ACTION_DOWN，AMOTION_EVENT_ACTION_UP，AMOTION_EVENT_ACTION_MOVE
    // position action动作对应的位置
    void setTouchEventData(quint32 id, AndroidMotioneventAction action, QRect position);
    void setScrollEventData(QRect position, qint32 hScroll, qint32 vScroll);
    void setCommandEventData(qint32 action);

    QByteArray serializeData();

private:
    void write32(QBuffer& buffer, quint32 value);
    void write16(QBuffer& buffer, quint32 value);
    void writePosition(QBuffer& buffer, const QRect& value);

private:
    struct ControlEventData {
        ControlEventType type;
        union {
            struct {
                AndroidKeyeventAction action;
                AndroidKeycode keycode;
                AndroidMetastate metastate;
            } keycodeEvent;
            struct {
                char text[TEXT_MAX_CHARACTER_LENGTH + 1];
            } textEvent;
            struct {
                AndroidMotioneventAction action;
                AndroidMotioneventButtons buttons;
                QRect position;
            } mouseEvent;
            struct {
                quint32 id;
                AndroidMotioneventAction action;
                QRect position;
            } touchEvent;
            struct {
                QRect position;
                qint32 hScroll;
                qint32 vScroll;
            } scrollEvent;
            struct {
                qint32 action;
            } commandEvent;
        };

        ControlEventData(){}
        ~ControlEventData(){}
    };

    ControlEventData m_data;
};

#endif // CONTROLEVENT_H
