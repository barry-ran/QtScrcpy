#include <QDebug>

#include "controlevent.h"

#define TEXT_MAX_CHARACTER_LENGTH 300

ControlEvent::ControlEvent(ControlEventType controlEventType)
    : QScrcpyEvent(Control)
{
    m_data.type = controlEventType;
}

void ControlEvent::setKeycodeEventData(AndroidKeyeventAction action, AndroidKeycode keycode, AndroidMetastate metastate)
{
    m_data.keycodeEvent.action = action;
    m_data.keycodeEvent.keycode = keycode;
    m_data.keycodeEvent.metastate = metastate;
}

void ControlEvent::setTextEventData(QString text)
{
    m_data.textEvent.text = text;
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

void ControlEvent::setCommandEventData(qint32 action)
{
    m_data.commandEvent.action = action;
}

void ControlEvent::write32(QBuffer &buffer, quint32 value)
{
    buffer.putChar(value >> 24);
    buffer.putChar(value >> 16);
    buffer.putChar(value >> 8);
    buffer.putChar(value);
}

void ControlEvent::write16(QBuffer &buffer, quint32 value)
{
    buffer.putChar(value >> 8);
    buffer.putChar(value);
}

void ControlEvent::writePosition(QBuffer &buffer, const QRect& value)
{
    write16(buffer, value.left());
    write16(buffer, value.top());
    write16(buffer, value.width());
    write16(buffer, value.height());
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
        write32(buffer, m_data.keycodeEvent.keycode);
        write32(buffer, m_data.keycodeEvent.metastate);
        break;
    case CET_TEXT:
    {
        // write length (2 byte) + date (non nul-terminated)
        if (TEXT_MAX_CHARACTER_LENGTH < m_data.textEvent.text.length()) {
            // injecting a text takes time, so limit the text length
            m_data.textEvent.text = m_data.textEvent.text.left(TEXT_MAX_CHARACTER_LENGTH);
        }
        QByteArray tmp = m_data.textEvent.text.toUtf8();
        write16(buffer, tmp.length());
        buffer.write(tmp.data(), tmp.length());
    }
        break;
    case CET_MOUSE:
        buffer.putChar(m_data.mouseEvent.action);
        write32(buffer, m_data.mouseEvent.buttons);
        writePosition(buffer, m_data.mouseEvent.position);
        break;
    case CET_TOUCH:
        buffer.putChar(m_data.touchEvent.id);
        buffer.putChar(m_data.touchEvent.action);
        writePosition(buffer, m_data.touchEvent.position);
        break;
    case CET_SCROLL:
        writePosition(buffer, m_data.scrollEvent.position);
        write32(buffer, m_data.scrollEvent.hScroll);
        write32(buffer, m_data.scrollEvent.vScroll);
        break;
    case CET_COMMAND:
        buffer.putChar(m_data.commandEvent.action);
        break;
    default:
        qDebug() << "Unknown event type:" << m_data.type;
        break;
    }
    buffer.close();
    return byteArray;
}
