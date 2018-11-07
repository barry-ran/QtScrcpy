#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QPointer>

class DeviceSocket;
class ControlEvent;
class Controller : public QObject
{
    Q_OBJECT
public:
    Controller(QObject* parent = Q_NULLPTR);
    virtual ~Controller();

    void setDeviceSocket(DeviceSocket* deviceSocket);
    void postControlEvent(ControlEvent* controlEvent);
    void test(QRect rc);

protected:
    bool event(QEvent *event);

private:
    bool sendControl(const QByteArray& buffer);

private:
    QPointer<DeviceSocket> m_deviceSocket;
};

#endif // CONTROLLER_H
