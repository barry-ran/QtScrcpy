#include <QDebug>

#include "deviceevent.h"
#include "bufferutil.h"

DeviceEvent::DeviceEvent(QObject *parent) : QObject(parent)
{

}

DeviceEvent::~DeviceEvent()
{
    if (DET_GET_CLIPBOARD == m_data.type
            && Q_NULLPTR != m_data.clipboardEvent.text) {
        delete m_data.clipboardEvent.text;
        m_data.clipboardEvent.text = Q_NULLPTR;
    }
}

DeviceEvent::DeviceEventType DeviceEvent::type()
{
    return m_data.type;
}

void DeviceEvent::getClipboardEventData(QString& text)
{
    text = QString::fromUtf8(m_data.clipboardEvent.text);
}

qint32 DeviceEvent::deserialize(QByteArray& byteArray)
{
    QBuffer buf(&byteArray);
    buf.open(QBuffer::ReadOnly);

    qint64 len = buf.size();
    char c = 0;
    qint32 ret = 0;

    if (len < 3) {
        // at least type + empty string length
        return 0; // not available
    }

    buf.getChar(&c);
    m_data.type = (DeviceEventType)c;
    switch (m_data.type) {
    case DET_GET_CLIPBOARD: {
        quint16 clipboardLen = BufferUtil::read16(buf);
        if (clipboardLen > len - 3) {
            ret = 0; // not available
            break;
        }

        QByteArray text = buf.readAll();
        m_data.clipboardEvent.text = new char[text.length() + 1];
        memcpy(m_data.clipboardEvent.text, text.data(), text.length());
        m_data.clipboardEvent.text[text.length()] = '\0';

        ret = 3 + clipboardLen;
        break;
    }
    default:
        qWarning("Unsupported device event type: %d", (int) m_data.type);
        ret = -1; // error, we cannot recover
    }

    buf.close();
    return ret;
}
