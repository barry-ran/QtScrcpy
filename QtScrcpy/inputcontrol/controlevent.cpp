#include <QDebug>

#include "controlevent.h"
#include "bufferutil.h"

ControlEvent::ControlEvent(ControlEventType controlEventType)
    : QScrcpyEvent(Control)
{
    m_data.type = controlEventType;
}

ControlEvent::~ControlEvent()
{
    if (CET_SET_CLIPBOARD == m_data.type
            && Q_NULLPTR != m_data.setClipboardEvent.text) {
        delete m_data.setClipboardEvent.text;
        m_data.setClipboardEvent.text = Q_NULLPTR;
    } else if (CET_TEXT == m_data.type
               && Q_NULLPTR != m_data.textEvent.text){
        delete m_data.textEvent.text;
        m_data.textEvent.text = Q_NULLPTR;
    }
}

void ControlEvent::setKeycodeEventData(AndroidKeyeventAction action, AndroidKeycode keycode, AndroidMetastate metastate)
{
    m_data.keycodeEvent.action = action;
    m_data.keycodeEvent.keycode = keycode;
    m_data.keycodeEvent.metastate = metastate;
}

void ControlEvent::setTextEventData(QString& text)
{
    // write length (2 byte) + string (non nul-terminated)
    if (CONTROL_EVENT_TEXT_MAX_LENGTH < text.length()) {
        // injecting a text takes time, so limit the text length
        text = text.left(CONTROL_EVENT_TEXT_MAX_LENGTH);
    }
    QByteArray tmp = text.toUtf8();
    m_data.textEvent.text = new char[tmp.length() + 1];
    memcpy(m_data.textEvent.text, tmp.data(), tmp.length());
    m_data.textEvent.text[tmp.length()] = '\0';
}

void ControlEvent::setMouseEventData(AndroidMotioneventAction action, AndroidMotioneventButtons buttons, QRect position)
{
    m_data.mouseEvent.action = action;
    m_data.mouseEvent.buttons = buttons;
    m_data.mouseEvent.position = position;
}

void ControlEvent::setTouchEventData(quint32 id, AndroidMotioneventAction action, QRect position)
{
    m_data.touchEvent.action = action;
    m_data.touchEvent.id = id;
    m_data.touchEvent.position = position;
}

void ControlEvent::setScrollEventData(QRect position, qint32 hScroll, qint32 vScroll)
{
    m_data.scrollEvent.position = position;
    m_data.scrollEvent.hScroll = hScroll;
    m_data.scrollEvent.vScroll = vScroll;
}

void ControlEvent::setSetClipboardEventData(QString &text)
{
    if (text.isEmpty()) {
        return;
    }
    if (CONTROL_EVENT_CLIPBOARD_TEXT_MAX_LENGTH < text.length()) {
        text = text.left(CONTROL_EVENT_CLIPBOARD_TEXT_MAX_LENGTH);
    }

    QByteArray tmp = text.toUtf8();
    m_data.setClipboardEvent.text = new char[tmp.length() + 1];
    memcpy(m_data.setClipboardEvent.text, tmp.data(), tmp.length());
    m_data.setClipboardEvent.text[tmp.length()] = '\0';
}

void ControlEvent::writePosition(QBuffer &buffer, const QRect& value)
{
    BufferUtil::write16(buffer, value.left());
    BufferUtil::write16(buffer, value.top());
    BufferUtil::write16(buffer, value.width());
    BufferUtil::write16(buffer, value.height());
}

QByteArray ControlEvent::serializeData()
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QBuffer::WriteOnly);
    buffer.putChar(m_data.type);

    switch (m_data.type) {
    case CET_KEYCODE:
        buffer.putChar(m_data.keycodeEvent.action);
        BufferUtil::write32(buffer, m_data.keycodeEvent.keycode);
        BufferUtil::write32(buffer, m_data.keycodeEvent.metastate);
        break;
    case CET_TEXT:
        BufferUtil::write16(buffer, strlen(m_data.textEvent.text));
        buffer.write(m_data.textEvent.text, strlen(m_data.textEvent.text));
        break;
    case CET_MOUSE:
        buffer.putChar(m_data.mouseEvent.action);
        BufferUtil::write32(buffer, m_data.mouseEvent.buttons);
        writePosition(buffer, m_data.mouseEvent.position);
        break;
    case CET_TOUCH:
        buffer.putChar(m_data.touchEvent.id);
        buffer.putChar(m_data.touchEvent.action);
        writePosition(buffer, m_data.touchEvent.position);
        break;
    case CET_SCROLL:
        writePosition(buffer, m_data.scrollEvent.position);
        BufferUtil::write32(buffer, m_data.scrollEvent.hScroll);
        BufferUtil::write32(buffer, m_data.scrollEvent.vScroll);
        break;
    case CET_SET_CLIPBOARD:
        BufferUtil::write16(buffer, strlen(m_data.setClipboardEvent.text));
        buffer.write(m_data.setClipboardEvent.text, strlen(m_data.setClipboardEvent.text));
        break;
    case CET_BACK_OR_SCREEN_ON:
    case CET_EXPAND_NOTIFICATION_PANEL:
    case CET_COLLAPSE_NOTIFICATION_PANEL:    
    case CET_GET_CLIPBOARD:
        break;
    default:
        qDebug() << "Unknown event type:" << m_data.type;
        break;
    }
    buffer.close();
    return byteArray;
}
