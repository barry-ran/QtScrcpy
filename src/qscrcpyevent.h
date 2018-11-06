#ifndef QSCRCPYEVENT_H
#define QSCRCPYEVENT_H
#include <QEvent>
#include <QMouseEvent>
class QScrcpyEvent : public QEvent
{
public:
    enum Type {
        DeviceSocket = QEvent::User + 1,
    };
    QScrcpyEvent(Type type) : QEvent(QEvent::Type(type)){}
};

class DeviceSocketEvent : public QScrcpyEvent
{
public:
    DeviceSocketEvent() : QScrcpyEvent(DeviceSocket){}
};
#endif // QSCRCPYEVENT_H
