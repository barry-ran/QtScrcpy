#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>

class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = nullptr);
    virtual ~TcpServer();

protected:
    virtual void incomingConnection(qintptr handle);

private:
    bool m_isVideoSocket = true;
};

#endif // TCPSERVER_H
