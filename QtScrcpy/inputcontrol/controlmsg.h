#ifndef CONTROLMSG_H
#define CONTROLMSG_H

#include <QRect>
#include <QString>
#include <QBuffer>

#include "qscrcpyevent.h"
#include "input.h"
#include "keycodes.h"

#define CONTROL_MSG_TEXT_MAX_LENGTH 300
#define CONTROL_MSG_CLIPBOARD_TEXT_MAX_LENGTH 4093
// ControlMsg
class ControlMsg : public QScrcpyEvent
{
public:    
    enum ControlMsgType {
        CMT_NULL = -1,
        CMT_INJECT_KEYCODE = 0,
        CMT_INJECT_TEXT,
        CMT_INJECT_MOUSE,
        CMT_INJECT_SCROLL,
        CMT_BACK_OR_SCREEN_ON,
        CMT_EXPAND_NOTIFICATION_PANEL,
        CMT_COLLAPSE_NOTIFICATION_PANEL,
        CMT_GET_CLIPBOARD,
        CMT_SET_CLIPBOARD,

        CMT_INJECT_TOUCH,
    };    

    ControlMsg(ControlMsgType controlMsgType);
    virtual ~ControlMsg();

    void setInjectKeycodeMsgData(AndroidKeyeventAction action, AndroidKeycode keycode, AndroidMetastate metastate);
    void setInjectTextMsgData(QString& text);
    void setInjectMouseMsgData(AndroidMotioneventAction action, AndroidMotioneventButtons buttons, QRect position);
    // id 代表一个触摸点，最多支持10个触摸点[0,9]
    // action 只能是AMOTION_EVENT_ACTION_DOWN，AMOTION_EVENT_ACTION_UP，AMOTION_EVENT_ACTION_MOVE
    // position action动作对应的位置
    void setInjectTouchMsgData(quint32 id, AndroidMotioneventAction action, QRect position);
    void setInjectScrollMsgData(QRect position, qint32 hScroll, qint32 vScroll);
    void setSetClipboardMsgData(QString& text);

    QByteArray serializeData();

private:    
    void writePosition(QBuffer& buffer, const QRect& value);

private:
    struct ControlMsgData {
        ControlMsgType type = CMT_NULL;
        union {
            struct {
                AndroidKeyeventAction action;
                AndroidKeycode keycode;
                AndroidMetastate metastate;
            } injectKeycodeMsg;
            struct {
                char* text = Q_NULLPTR;
            } injectTextMsg;
            struct {
                AndroidMotioneventAction action;
                AndroidMotioneventButtons buttons;
                QRect position;
            } injectMouseMsg;
            struct {
                quint32 id;
                AndroidMotioneventAction action;
                QRect position;
            } injectTouchMsg;
            struct {
                QRect position;
                qint32 hScroll;
                qint32 vScroll;
            } injectScrollMsg;
            struct {
                char *text = Q_NULLPTR;
            } setClipboardMsg;
        };

        ControlMsgData(){}
        ~ControlMsgData(){}
    };

    ControlMsgData m_data;
};

#endif // CONTROLMSG_H
