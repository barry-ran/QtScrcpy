#ifndef DEVICESOCKET_H
#define DEVICESOCKET_H

#include <QTcpSocket>

class DeviceSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit DeviceSocket(QObject *parent = nullptr);

signals:

public slots:
};

#endif // DEVICESOCKET_H
