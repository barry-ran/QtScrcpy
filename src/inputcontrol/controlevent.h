#ifndef CONTROLEVENT_H
#define CONTROLEVENT_H

#include <QRect>
#include <QString>
#include <QBuffer>

#include "qscrcpyevent.h"
#include "input.h"
#include "keycodes.h"

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
    };   

    ControlEvent(ControlEventType controlEventType);

    void setKeycodeEventData(AndroidKeyeventAction action, AndroidKeycode keycode, AndroidMetastate metastate);
    void setTextEventData(QString text);
    void setMouseEventData(AndroidMotioneventAction action, AndroidMotioneventButtons buttons, QRect position);
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
                QString text;
            } textEvent;
            struct {
                AndroidMotioneventAction action;
                AndroidMotioneventButtons buttons;
                QRect position;
            } mouseEvent;
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
