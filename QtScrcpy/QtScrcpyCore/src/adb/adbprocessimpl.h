#pragma once

#include <QProcess>
#include "adbprocess.h"

class AdbProcessImpl : public QProcess
{
    Q_OBJECT

public:
    explicit AdbProcessImpl(QObject *parent = nullptr);
    virtual ~AdbProcessImpl();

    void execute(const QString &serial, const QStringList &args);
    void forward(const QString &serial, quint16 localPort, const QString &deviceSocketName);
    void forwardRemove(const QString &serial, quint16 localPort);
    void reverse(const QString &serial, const QString &deviceSocketName, quint16 localPort);
    void reverseRemove(const QString &serial, const QString &deviceSocketName);
    void push(const QString &serial, const QString &local, const QString &remote);
    void install(const QString &serial, const QString &local);
    void removePath(const QString &serial, const QString &path);
    bool isRuning();
    void setShowTouchesEnabled(const QString &serial, bool enabled);
    QStringList getDevicesSerialFromStdOut();
    QString getDeviceIPFromStdOut();
    QString getDeviceIPByIpFromStdOut();
    QString getStdOut();
    QString getErrorOut();

    static const QString &getAdbPath();

signals:
    void adbProcessImplResult(qsc::AdbProcess::ADB_EXEC_RESULT processResult);

private:
    void initSignals();

private:
    QString m_standardOutput = "";
    QString m_errorOutput = "";
    static QString s_adbPath;
};
