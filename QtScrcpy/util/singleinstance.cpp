#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#ifdef Q_OS_WIN32
#include <Windows.h>
#endif

#include "singleinstance.h"

namespace
{
    const char kActivateMessage[] = "activate";
    const int kTimeoutMs = 1000;

    // The name becomes a named pipe on Windows and a file under /tmp elsewhere,
    // so keep it to plain characters. Include the user name so two users on the
    // same machine each get their own instance.
    QString makeServerName(const QString &appId)
    {
        QString user = qEnvironmentVariable("USERNAME");
        if (user.isEmpty()) {
            user = qEnvironmentVariable("USER");
        }
        QString name = appId + "-" + user;
        for (QChar &c : name) {
            if (!c.isLetterOrNumber() && c != '-' && c != '_') {
                c = '_';
            }
        }
        return name;
    }
}

SingleInstance::SingleInstance(const QString &appId, QObject *parent) : QObject(parent), m_serverName(makeServerName(appId))
{
    // Probe for a live primary. A stale socket file left by a crashed primary
    // (Unix) refuses the connection, so a failed connect means we can take over.
    QLocalSocket probe;
    probe.connectToServer(m_serverName);
    if (probe.waitForConnected(kTimeoutMs)) {
        probe.disconnectFromServer();
        m_primary = false;
        return;
    }

    QLocalServer::removeServer(m_serverName);
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    if (m_server->listen(m_serverName)) {
        connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::onNewConnection);
    } else {
        // Better to run unguarded than to refuse to start at all.
        qWarning() << "SingleInstance: cannot listen on" << m_serverName << ":" << m_server->errorString();
        delete m_server;
        m_server = nullptr;
    }
    m_primary = true;
}

SingleInstance::~SingleInstance()
{
    if (m_server) {
        m_server->close(); // also removes the socket file on Unix
    }
}

bool SingleInstance::isPrimary() const
{
    return m_primary;
}

bool SingleInstance::notifyPrimary()
{
    QLocalSocket socket;
    socket.connectToServer(m_serverName);
    if (!socket.waitForConnected(kTimeoutMs)) {
        qWarning() << "SingleInstance: cannot reach the running instance:" << socket.errorString();
        return false;
    }

#ifdef Q_OS_WIN32
    // This process was just launched by the user, so it holds the right to
    // change the foreground window. Hand that right to the primary; without it
    // Windows only flashes the taskbar button instead of raising the window.
    AllowSetForegroundWindow(ASFW_ANY);
#endif

    socket.write(kActivateMessage);
    socket.waitForBytesWritten(kTimeoutMs);
    socket.disconnectFromServer();
    if (socket.state() != QLocalSocket::UnconnectedState) {
        socket.waitForDisconnected(kTimeoutMs);
    }
    return true;
}

void SingleInstance::onNewConnection()
{
    while (QLocalSocket *client = m_server->nextPendingConnection()) {
        connect(client, &QLocalSocket::disconnected, client, &QLocalSocket::deleteLater);
        connect(client, &QLocalSocket::readyRead, this, [this, client]() {
            if (client->readAll().contains(kActivateMessage)) {
                emit activateRequested();
            }
            client->disconnectFromServer();
        });
    }
}
