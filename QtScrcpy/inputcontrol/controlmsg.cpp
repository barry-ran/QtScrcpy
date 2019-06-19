#include <QDebug>

#include "controlmsg.h"
#include "bufferutil.h"

ControlMsg::ControlMsg(ControlMsgType controlMsgType)
    : QScrcpyEvent(Control)
{
    m_data.type = controlMsgType;
}

ControlMsg::~ControlMsg()
{
    if (CMT_SET_CLIPBOARD == m_data.type
            && Q_NULLPTR != m_data.setClipboardMsg.text) {
        delete m_data.setClipboardMsg.text;
        m_data.setClipboardMsg.text = Q_NULLPTR;
    } else if (CMT_INJECT_TEXT == m_data.type
               && Q_NULLPTR != m_data.injectTextMsg.text){
        delete m_data.injectTextMsg.text;
        m_data.injectTextMsg.text = Q_NULLPTR;
    }
}

void ControlMsg::setInjectKeycodeMsgData(AndroidKeyeventAction action, AndroidKeycode keycode, AndroidMetastate metastate)
{
    m_data.injectKeycodeMsg.action = action;
    m_data.injectKeycodeMsg.keycode = keycode;
    m_data.injectKeycodeMsg.metastate = metastate;
}

void ControlMsg::setInjectTextMsgData(QString& text)
{
    // write length (2 byte) + string (non nul-terminated)
    if (CONTROL_MSG_TEXT_MAX_LENGTH < text.length()) {
        // injecting a text takes time, so limit the text length
        text = text.left(CONTROL_MSG_TEXT_MAX_LENGTH);
    }
    QByteArray tmp = text.toUtf8();
    m_data.injectTextMsg.text = new char[tmp.length() + 1];
    memcpy(m_data.injectTextMsg.text, tmp.data(), tmp.length());
    m_data.injectTextMsg.text[tmp.length()] = '\0';
}

void ControlMsg::setInjectMouseMsgData(AndroidMotioneventAction action, AndroidMotioneventButtons buttons, QRect position)
{
    m_data.injectMouseMsg.action = action;
    m_data.injectMouseMsg.buttons = buttons;
    m_data.injectMouseMsg.position = position;
}

void ControlMsg::setInjectTouchMsgData(quint32 id, AndroidMotioneventAction action, QRect position)
{
    m_data.injectTouchMsg.action = action;
    m_data.injectTouchMsg.id = id;
    m_data.injectTouchMsg.position = position;
}

void ControlMsg::setInjectScrollMsgData(QRect position, qint32 hScroll, qint32 vScroll)
{
    m_data.injectScrollMsg.position = position;
    m_data.injectScrollMsg.hScroll = hScroll;
    m_data.injectScrollMsg.vScroll = vScroll;
}

void ControlMsg::setSetClipboardMsgData(QString &text)
{
    if (text.isEmpty()) {
        return;
    }
    if (CONTROL_MSG_CLIPBOARD_TEXT_MAX_LENGTH < text.length()) {
        text = text.left(CONTROL_MSG_CLIPBOARD_TEXT_MAX_LENGTH);
    }

    QByteArray tmp = text.toUtf8();
    m_data.setClipboardMsg.text = new char[tmp.length() + 1];
    memcpy(m_data.setClipboardMsg.text, tmp.data(), tmp.length());
    m_data.setClipboardMsg.text[tmp.length()] = '\0';
}

void ControlMsg::writePosition(QBuffer &buffer, const QRect& value)
{
    BufferUtil::write16(buffer, value.left());
    BufferUtil::write16(buffer, value.top());
    BufferUtil::write16(buffer, value.width());
    BufferUtil::write16(buffer, value.height());
}

QByteArray ControlMsg::serializeData()
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QBuffer::WriteOnly);
    buffer.putChar(m_data.type);

    switch (m_data.type) {
    case CMT_INJECT_KEYCODE:
        buffer.putChar(m_data.injectKeycodeMsg.action);
        BufferUtil::write32(buffer, m_data.injectKeycodeMsg.keycode);
        BufferUtil::write32(buffer, m_data.injectKeycodeMsg.metastate);
        break;
    case CMT_INJECT_TEXT:
        BufferUtil::write16(buffer, strlen(m_data.injectTextMsg.text));
        buffer.write(m_data.injectTextMsg.text, strlen(m_data.injectTextMsg.text));
        break;
    case CMT_INJECT_MOUSE:
        buffer.putChar(m_data.injectMouseMsg.action);
        BufferUtil::write32(buffer, m_data.injectMouseMsg.buttons);
        writePosition(buffer, m_data.injectMouseMsg.position);
        break;
    case CMT_INJECT_TOUCH:
        buffer.putChar(m_data.injectTouchMsg.id);
        buffer.putChar(m_data.injectTouchMsg.action);
        writePosition(buffer, m_data.injectTouchMsg.position);
        break;
    case CMT_INJECT_SCROLL:
        writePosition(buffer, m_data.injectScrollMsg.position);
        BufferUtil::write32(buffer, m_data.injectScrollMsg.hScroll);
        BufferUtil::write32(buffer, m_data.injectScrollMsg.vScroll);
        break;
    case CMT_SET_CLIPBOARD:
        BufferUtil::write16(buffer, strlen(m_data.setClipboardMsg.text));
        buffer.write(m_data.setClipboardMsg.text, strlen(m_data.setClipboardMsg.text));
        break;
    case CMT_BACK_OR_SCREEN_ON:
    case CMT_EXPAND_NOTIFICATION_PANEL:
    case CMT_COLLAPSE_NOTIFICATION_PANEL:
    case CMT_GET_CLIPBOARD:
        break;
    default:
        qDebug() << "Unknown event type:" << m_data.type;
        break;
    }
    buffer.close();
    return byteArray;
}
