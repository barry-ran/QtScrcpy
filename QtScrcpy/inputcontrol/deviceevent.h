#ifndef DEVICEEVENT_H
#define DEVICEEVENT_H

#include <QBuffer>

#define DEVICE_EVENT_QUEUE_SIZE 64
#define DEVICE_EVENT_TEXT_MAX_LENGTH 4093
#define DEVICE_EVENT_SERIALIZED_MAX_SIZE (3 + DEVICE_EVENT_TEXT_MAX_LENGTH)

class DeviceEvent : public QObject
{
    Q_OBJECT
public:
    enum DeviceEventType {
        DET_NULL = -1,
        // 和服务端对应
        DET_GET_CLIPBOARD = 0,
    };
    explicit DeviceEvent(QObject *parent = nullptr);
    virtual ~DeviceEvent();

    DeviceEvent::DeviceEventType type();
    void getClipboardEventData(QString& text);

    qint32 deserialize(QByteArray& byteArray);

private:
    struct DeviceEventData {
        DeviceEventType type = DET_NULL;
        union {
            struct {                
                char* text = Q_NULLPTR;
            } clipboardEvent;
        };
        DeviceEventData(){}
        ~DeviceEventData(){}
    };

    DeviceEventData m_data;
};

#endif // DEVICEEVENT_H
