#ifndef RECEIVER_H
#define RECEIVER_H

#include <QPointer>

class Controller;
class DeviceEvent;
class Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Receiver(Controller *controller);
    virtual ~Receiver();

public slots:
    void onReadyRead();

protected:
    void processEvent(DeviceEvent *deviceEvent);

private:
    QPointer<Controller> m_controller;
};

#endif // RECEIVER_H
