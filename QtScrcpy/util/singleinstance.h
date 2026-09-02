#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QObject>
#include <QString>

class QLocalServer;

// Keeps a single QtScrcpy process per user.
//
// The first process becomes the primary and listens on a local socket. Any
// process started later finds that socket, asks the primary to show its main
// window (which may be hidden in the system tray) and then exits.
class SingleInstance : public QObject
{
    Q_OBJECT
public:
    explicit SingleInstance(const QString &appId, QObject *parent = nullptr);
    ~SingleInstance() override;

    // true when no other instance was found and this process now owns the lock
    bool isPrimary() const;

    // secondary instance only: ask the primary to show its window
    bool notifyPrimary();

signals:
    // primary instance only: a secondary instance asked us to show the window
    void activateRequested();

private:
    void onNewConnection();

private:
    QString m_serverName;
    QLocalServer *m_server = nullptr;
    bool m_primary = false;
};

#endif // SINGLEINSTANCE_H
