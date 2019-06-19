#ifndef RECEIVER_H
#define RECEIVER_H

#include <QPointer>

class Controller;
class DeviceMsg;
class Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Receiver(Controller *controller);
    virtual ~Receiver();

public slots:
    void onReadyRead();

protected:
    void processMsg(DeviceMsg *deviceMsg);

private:
    QPointer<Controller> m_controller;
};

#endif // RECEIVER_H
