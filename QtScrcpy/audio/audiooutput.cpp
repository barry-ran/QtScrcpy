#include <QTcpSocket>
#include <QHostAddress>
#include <QAudioSink>
#include <QMediaDevices>
#include <QElapsedTimer>
#include <QProcess>
#include <QThread>
#include <QDebug>
#include <QByteArray>
#include "audiooutput.h"

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent)
    , m_outputDevice(nullptr)
    , m_running(false)
    , m_audioSink(nullptr)
{
    connect(&m_sndcpy, &QProcess::readyReadStandardOutput, this, [this]() {
        qInfo() << QString("AudioOutput::") << m_sndcpy.readAllStandardOutput();
    });
    connect(&m_sndcpy, &QProcess::readyReadStandardError, this, [this]() {
        qInfo() << QString("AudioOutput::") << m_sndcpy.readAllStandardError();
    });
}

AudioOutput::~AudioOutput()
{
    if (m_sndcpy.state() != QProcess::NotRunning) {
        m_sndcpy.kill();
    }
    stop();
}

bool AudioOutput::start(const QString &serial, int port)
{
    if (m_running) {
        stop();
    }

    QElapsedTimer timeConsumeCount;
    timeConsumeCount.start();
    bool ret = runSndcpyProcess(serial, port);
    qInfo() << "AudioOutput::run sndcpy cost:" << timeConsumeCount.elapsed() << "milliseconds";
    if (!ret) {
        return false;
    }

    startAudioOutput();
    startRecvData(port);

    m_running = true;
    return true;
}

void AudioOutput::stop()
{
    if (!m_running) {
        return;
    }
    m_running = false;

    stopRecvData();
    stopAudioOutput();
}

void AudioOutput::installonly(const QString &serial, int port)
{
    runSndcpyProcess(serial, port, false);
}

bool AudioOutput::runSndcpyProcess(const QString &serial, int port, bool wait)
{
    if (m_sndcpy.state() != QProcess::NotRunning) {
        m_sndcpy.kill();
    }

#ifdef Q_OS_WIN32
    QStringList params{serial, QString::number(port)};
    m_sndcpy.start("sndcpy.bat", params);
#else
    QStringList params{"sndcpy.sh", serial, QString::number(port)};
    m_sndcpy.start("bash", params);
#endif

    if (!wait) {
        return true;
    }

    if (!m_sndcpy.waitForStarted()) {
        qWarning() << "AudioOutput::start sndcpy process failed";
        return false;
    }
    if (!m_sndcpy.waitForFinished()) {
        qWarning() << "AudioOutput::sndcpy process crashed";
        return false;
    }

    return true;
}

void AudioOutput::startAudioOutput()
{
    if (m_audioSink) {
        return;
    }

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16); // 16-bit signed integer format

    QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
    if (!defaultDevice.isFormatSupported(format)) {
        qWarning() << "AudioOutput::audio format not supported, cannot play audio.";
        return;
    }

    m_audioSink = new QAudioSink(defaultDevice, format, this);
    m_outputDevice = m_audioSink->start();
    if (!m_outputDevice) {
        qWarning() << "AudioOutput::failed to start audio sink.";
        delete m_audioSink;
        m_audioSink = nullptr;
        return;
    }
}

void AudioOutput::stopAudioOutput()
{
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    m_outputDevice = nullptr;
}

void AudioOutput::startRecvData(int port)
{
    if (m_workerThread.isRunning()) {
        stopRecvData();
    }

    auto audioSocket = new QTcpSocket();
    audioSocket->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, audioSocket, &QObject::deleteLater);

    connect(this, &AudioOutput::connectTo, audioSocket, [audioSocket](int port) {
        audioSocket->connectToHost(QHostAddress::LocalHost, port);
        if (!audioSocket->waitForConnected(500)) {
            qWarning("AudioOutput::audio socket connect failed");
            return;
        }
        qInfo("AudioOutput::audio socket connect success");
    });
    connect(audioSocket, &QIODevice::readyRead, audioSocket, [this, audioSocket]() {
        qint64 recv = audioSocket->bytesAvailable();

        if (!m_outputDevice) {
            return;
        }
        if (m_buffer.capacity() < recv) {
            m_buffer.reserve(recv);
        }

        qint64 count = audioSocket->read(m_buffer.data(), recv);
        m_outputDevice->write(m_buffer.data(), count);
    });
    connect(audioSocket, &QTcpSocket::stateChanged, audioSocket, [](QAbstractSocket::SocketState state) {
        qInfo() << "AudioOutput::audio socket state changed:" << state;
    });

    connect(audioSocket, &QTcpSocket::errorOccurred, audioSocket, [](QAbstractSocket::SocketError error) {
        qInfo() << "AudioOutput::audio socket error occurred:" << error;
    });

    m_workerThread.start();
    emit connectTo(port);
}

void AudioOutput::stopRecvData()
{
    if (!m_workerThread.isRunning()) {
        return;
    }

    m_workerThread.quit();
    m_workerThread.wait();
}
