#ifndef QSCRCPYEVENT_H
#define QSCRCPYEVENT_H
#include <QEvent>

class QScrcpyEvent : public QEvent
{
public:
    enum Type
    {
        VideoSocket = QEvent::User + 1,
        Control,
    };
    QScrcpyEvent(Type type) : QEvent(QEvent::Type(type)) {}
};

// VideoSocketEvent
class VideoSocketEvent : public QScrcpyEvent
{
public:
    VideoSocketEvent() : QScrcpyEvent(VideoSocket) {}
};
#endif // QSCRCPYEVENT_H
