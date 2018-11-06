#ifndef QSCRCPYEVENT_H
#define QSCRCPYEVENT_H
#include <QEvent>

class QScrcpyEvent : public QEvent
{
public:
    enum Type {
        DeviceSocket = QEvent::User + 1,
        Control,
    };
    QScrcpyEvent(Type type) : QEvent(QEvent::Type(type)){}
};

// DeviceSocketEvent
class DeviceSocketEvent : public QScrcpyEvent
{
public:
    DeviceSocketEvent() : QScrcpyEvent(DeviceSocket){}
};
#endif // QSCRCPYEVENT_H
