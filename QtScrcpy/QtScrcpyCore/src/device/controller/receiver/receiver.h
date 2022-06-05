#ifndef RECEIVER_H
#define RECEIVER_H

#include <QPointer>

class DeviceMsg;
class Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Receiver(QObject *parent = Q_NULLPTR);
    virtual ~Receiver();

    void recvDeviceMsg(DeviceMsg *deviceMsg);
};

#endif // RECEIVER_H
