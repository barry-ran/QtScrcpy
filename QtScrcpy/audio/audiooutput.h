#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include "pcmbuffer.h"
#include <QProcess>
#include <QPointer>
#include <QVector>
#include <QTimer>
#include <QThread>
class AudioStream;

class QTcpSocket;
class QAudioSink;
class QAudioOutput;
class QIODevice;
class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(QObject *parent = nullptr);
    ~AudioOutput();

    bool start(const QString& serial, int port);
    void stop();
    void stopForDevice(const QString &serial);
    bool isActive() const { return m_running||m_starting; }
    bool isStarting() const { return m_starting; }
    QString serial() const { return m_serial; }

signals:
    void stateChanged();

public:
    void installonly(const QString& serial, int port);

private:
    bool runSndcpyProcess(const QString& serial, int port);
    QThread m_audioThread;
    AudioStream *m_stream = nullptr;
    QProcess m_sndcpy;
    bool m_running = false;
    QString m_serial;
    bool m_starting = false;
    int m_pendingPort = 28200;
    QTimer m_startTimeout;

};

#endif // AUDIOOUTPUT_H
