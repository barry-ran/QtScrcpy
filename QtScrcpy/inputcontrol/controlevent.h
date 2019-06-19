#ifndef CONTROLEVENT_H
#define CONTROLEVENT_H

#include <QRect>
#include <QString>
#include <QBuffer>

#include "qscrcpyevent.h"
#include "input.h"
#include "keycodes.h"

#define CONTROL_EVENT_TEXT_MAX_LENGTH 300
#define CONTROL_EVENT_CLIPBOARD_TEXT_MAX_LENGTH 4093
// ControlEvent
class ControlEvent : public QScrcpyEvent
{
public:    
    enum ControlEventType {
        CET_NULL = -1,
        CET_KEYCODE = 0,
        CET_TEXT,
        CET_MOUSE,
        CET_SCROLL,        
        CET_BACK_OR_SCREEN_ON,
        CET_EXPAND_NOTIFICATION_PANEL,
        CET_COLLAPSE_NOTIFICATION_PANEL,
        CET_GET_CLIPBOARD,
        CET_SET_CLIPBOARD,

        CET_TOUCH,
    };    

    ControlEvent(ControlEventType controlEventType);
    virtual ~ControlEvent();

    void setKeycodeEventData(AndroidKeyeventAction action, AndroidKeycode keycode, AndroidMetastate metastate);
    void setTextEventData(QString& text);
    void setMouseEventData(AndroidMotioneventAction action, AndroidMotioneventButtons buttons, QRect position);
    // id 代表一个触摸点，最多支持10个触摸点[0,9]
    // action 只能是AMOTION_EVENT_ACTION_DOWN，AMOTION_EVENT_ACTION_UP，AMOTION_EVENT_ACTION_MOVE
    // position action动作对应的位置
    void setTouchEventData(quint32 id, AndroidMotioneventAction action, QRect position);
    void setScrollEventData(QRect position, qint32 hScroll, qint32 vScroll);
    void setSetClipboardEventData(QString& text);

    QByteArray serializeData();

private:    
    void writePosition(QBuffer& buffer, const QRect& value);

private:
    struct ControlEventData {
        ControlEventType type = CET_NULL;
        union {
            struct {
                AndroidKeyeventAction action;
                AndroidKeycode keycode;
                AndroidMetastate metastate;
            } keycodeEvent;
            struct {
                char* text = Q_NULLPTR;
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
                char *text = Q_NULLPTR;
            } setClipboardEvent;
        };

        ControlEventData(){}
        ~ControlEventData(){}
    };

    ControlEventData m_data;
};

#endif // CONTROLEVENT_H
