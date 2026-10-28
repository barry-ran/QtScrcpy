#include <QAudioOutput>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTime>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QAudioSink>
#include <QAudioDevice>
#include <QMediaDevices>
#endif

#include "audiooutput.h"

class AudioStream : public QObject {
public:
    AudioStream() {
        m_pumpTimer.setParent(this);
        m_pumpTimer.setInterval(5);
        m_pumpTimer.setTimerType(Qt::PreciseTimer);
        connect(&m_pumpTimer, &QTimer::timeout, this, &AudioStream::pumpAudio);
    }
    void start(int port) { startAudioOutput(); startRecvData(port); }
    void stop() { stopRecvData(); stopAudioOutput(); }
private:
    void startAudioOutput();
    void stopAudioOutput();
    void startRecvData(int port);
    void stopRecvData();
    void pumpAudio();
    QPointer<QIODevice> m_outputDevice;
    QPointer<QTcpSocket> m_audioSocket;
    QTimer m_pumpTimer;
    PcmBuffer m_pcm;
    bool m_prebuffered = false;
    bool m_hasPlayed = false;
    int m_underflows = 0;
    QElapsedTimer m_reportClock;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QAudioOutput *m_audioOutput = nullptr;
#else
    QAudioSink *m_audioSink = nullptr;
#endif
};

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent)
{
    m_running = false;
    m_stream = new AudioStream;
    m_stream->moveToThread(&m_audioThread);
    connect(&m_audioThread, &QThread::finished, m_stream, &QObject::deleteLater);
    m_audioThread.start(QThread::HighPriority);
    connect(&m_sndcpy, &QProcess::readyReadStandardOutput, this, [this]() {
        qInfo() << QString("AudioOutput::") << QString(m_sndcpy.readAllStandardOutput());
    });
    connect(&m_sndcpy, &QProcess::readyReadStandardError, this, [this]() {
        qInfo() << QString("AudioOutput::") << QString(m_sndcpy.readAllStandardError());
    });
    m_startTimeout.setSingleShot(true);
    connect(&m_startTimeout, &QTimer::timeout, this, [this]() {
        m_starting = false;emit stateChanged();
        m_sndcpy.kill();
        qWarning() << "音频启动超时，请检查手机授权；投屏保持运行。";
    });
    connect(&m_sndcpy, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
        m_startTimeout.stop();
        if (!m_starting) return;
        m_starting = false;emit stateChanged();
        if (code != 0 || status != QProcess::NormalExit) {
            qWarning() << "音频启动失败；投屏保持运行。";
            return;
        }
        const int port = m_pendingPort;
        QMetaObject::invokeMethod(m_stream, [this, port]() { m_stream->start(port); }, Qt::QueuedConnection);
        m_running = true;emit stateChanged();
    });
    connect(&m_sndcpy, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_starting = false;emit stateChanged(); m_startTimeout.stop();
            qWarning() << "无法启动音频脚本。";
        }
    });
}

AudioOutput::~AudioOutput()
{
    if (QProcess::NotRunning != m_sndcpy.state()) {
        m_sndcpy.kill();
    }
    stop();
    m_audioThread.quit();
    m_audioThread.wait();
}

bool AudioOutput::start(const QString& serial, int port)
{
    if (m_starting || m_sndcpy.state() != QProcess::NotRunning) {
        qWarning() << "音频正在启动，请等待手机授权。";
        return false;
    }
    stop();
    m_pendingPort = port;
    m_serial=serial;
    m_starting = true;emit stateChanged();
    runSndcpyProcess(serial, port);
    m_startTimeout.start(20000);
    return true;
}

void AudioOutput::stopForDevice(const QString &serial)
{
    if(!serial.isEmpty()&&m_serial==serial)stop();
}

void AudioOutput::stop()
{
    m_starting=false;m_running=false;m_startTimeout.stop();
    if(m_sndcpy.state()!=QProcess::NotRunning) {
        m_sndcpy.kill();m_sndcpy.waitForFinished(1000);
    }
    if(m_audioThread.isRunning())
        QMetaObject::invokeMethod(m_stream,[this](){m_stream->stop();},Qt::BlockingQueuedConnection);
    m_serial.clear();emit stateChanged();
}

void AudioOutput::installonly(const QString &serial, int port)
{
    runSndcpyProcess(serial, port);
}

bool AudioOutput::runSndcpyProcess(const QString &serial, int port)
{
    if (QProcess::NotRunning != m_sndcpy.state()) {
        m_sndcpy.kill();
    }

#ifdef Q_OS_WIN32
    QStringList params{serial, QString::number(port)};
    m_sndcpy.start("sndcpy.bat", params);
#else
    QStringList params{"sndcpy.sh", serial, QString::number(port)};
    m_sndcpy.setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_sndcpy.start("bash", params);
#endif

    return true;
}

void AudioStream::startAudioOutput()
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    if (m_audioOutput) {
        return;
    }

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());

    if (!info.isFormatSupported(format)) {
        qWarning() << "AudioOutput::audio format not supported, cannot play audio.";
        return;
    }

    m_audioOutput = new QAudioOutput(format, this);
    connect(m_audioOutput, &QAudioOutput::stateChanged, this, [](QAudio::State state) {
        qInfo() << "AudioOutput::audio state changed:" << state;
    });
    m_audioOutput->setBufferSize(48000*2*15/1000 * 20);
    m_outputDevice = m_audioOutput->start();
#else
    if (m_audioSink) {
        return;
    }

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
    if (!defaultDevice.isFormatSupported(format)) {
        qWarning() << "AudioOutput::audio format not supported, cannot play audio.";
        return;
    }
    m_audioSink = new QAudioSink(defaultDevice, format, this);
    m_audioSink->setBufferSize(192 * 80);
    connect(m_audioSink, &QAudioSink::stateChanged, this, [this](QtAudio::State state) {
        if (state == QtAudio::IdleState && m_hasPlayed) {
            ++m_underflows;
            m_prebuffered = false;
        }
    });
    m_outputDevice = m_audioSink->start();
    if (!m_outputDevice) {
        qWarning() << "AudioOutput::audio output device not available, cannot play audio.";
        delete m_audioSink;
        m_audioSink = nullptr;
        return;
    }
#endif
}

void AudioStream::stopAudioOutput()
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    if (m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
        m_audioOutput = nullptr;
    }
#else
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
#endif
    m_outputDevice = nullptr;
}

void AudioStream::startRecvData(int port)
{
    stopRecvData();
    // Socket, queue and QAudioSink all belong to this thread.
    m_audioSocket = new QTcpSocket(this);
    auto *socket = m_audioSocket.data();
    socket->setReadBufferSize(192 * 250);
    connect(socket, &QTcpSocket::connected, this, [socket]() {
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        qInfo() << "音频连接成功：48 kHz / 16-bit / stereo，80 ms 播放缓冲";
    });
    connect(socket, &QIODevice::readyRead, this, [this, socket]() {
        m_pcm.append(socket->readAll());
        pumpAudio();
    });
    connect(socket, &QTcpSocket::disconnected, this, [this]() {
        m_pcm.clear(); m_prebuffered = false;
        qWarning() << "音频连接断开。";
    });
    connect(socket, &QTcpSocket::errorOccurred, this, [](QAbstractSocket::SocketError error) {
        qWarning() << "音频连接错误：" << error;
    });
    socket->connectToHost(QHostAddress::LocalHost, port);
    m_hasPlayed = false; m_underflows = 0; m_reportClock.start();
    m_pumpTimer.start();
}

void AudioStream::pumpAudio()
{
    if (m_reportClock.elapsed() >= 10000) {
        qInfo() << "Audio playback: underflows=" << m_underflows << "queuedMs=" << m_pcm.size() / 192;
        m_reportClock.restart();
    }

    if (!m_outputDevice) return;
    if (!m_prebuffered) {
        if (m_pcm.size() < 192 * 40) return;
        m_prebuffered = true;
    }
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    const qint64 freeBytes = m_audioOutput ? m_audioOutput->bytesFree() : 0;
#else
    const qint64 freeBytes = m_audioSink ? m_audioSink->bytesFree() : 0;
#endif
    if (m_pcm.drain(m_outputDevice, freeBytes) > 0) m_hasPlayed = true;
}

void AudioStream::stopRecvData()
{
    m_pumpTimer.stop();
    if (m_audioSocket) {
        m_audioSocket->disconnect(this);
        m_audioSocket->abort();
        m_audioSocket->deleteLater();
        m_audioSocket = nullptr;
    }
    m_pcm.clear(); m_prebuffered = false;
}
