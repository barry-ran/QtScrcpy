#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QThread>
#include <QProcess>
#include <QPointer>
#include <QVector>

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
    bool runSndcpyProcess(const QString& serial, int port, bool wait = true);
    void startAudioOutput();
    void stopAudioOutput();
    void startRecvData(int port);
    void stopRecvData();

signals:
    void connectTo(int port);

private:
    QAudioOutput* m_audioOutput = nullptr;
    QPointer<QIODevice> m_outputDevice;
    QThread m_workerThread;
    QProcess m_sndcpy;
    QVector<char> m_buffer;
    bool m_running = false;
};

#endif // AUDIOOUTPUT_H
