#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QThread>
#include <QProcess>
#include <QPointer>
#include <QByteArray>
#include <QTimer>

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
    void installonly(const QString& serial, int port);

private:
    bool runSndcpyProcess(const QString& serial, int port, bool wait = false);
    bool startAudioOutput();
    void stopAudioOutput();
    void startRecvData(int port);
    void stopRecvData();

private slots:
    void writeAudioData(const QByteArray &data);

signals:
    void connectTo(int port);
    void audioDataReceived(const QByteArray &data);

private:
    QPointer<QIODevice> m_outputDevice;
    QThread m_workerThread;
    QProcess m_sndcpy;
    QTimer m_startupTimer;
    bool m_running = false;
    bool m_startPending = false;
    int m_pendingPort = 0;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QAudioOutput* m_audioOutput = nullptr;
#else
    QAudioSink *m_audioSink = nullptr;
#endif
};

#endif // AUDIOOUTPUT_H
