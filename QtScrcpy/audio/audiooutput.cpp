#include <QAudioOutput>
#include <QCoreApplication>
#include <QDir>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QAudioSink>
#include <QAudioDevice>
#include <QMediaDevices>
#endif

#include "audiooutput.h"

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent)
{
    m_startupTimer.setSingleShot(true);
    m_running = false;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    m_audioOutput = nullptr;
#else
    m_audioSink = nullptr;
#endif
    connect(&m_sndcpy, &QProcess::readyReadStandardOutput, this, [this]() {
        qInfo() << QString("AudioOutput::") << QString(m_sndcpy.readAllStandardOutput());
    });
    connect(&m_sndcpy, &QProcess::readyReadStandardError, this, [this]() {
        qInfo() << QString("AudioOutput::") << QString(m_sndcpy.readAllStandardError());
    });
    connect(&m_sndcpy, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_startupTimer.stop();
        if (!m_startPending) {
            return;
        }
        m_startPending = false;
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            qWarning() << "AudioOutput::sndcpy setup failed with exit code" << exitCode;
            stopAudioOutput();
            return;
        }
        m_running = true;
        startRecvData(m_pendingPort);
    });
    connect(&m_sndcpy, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_startPending) {
            return;
        }
        m_startupTimer.stop();
        m_startPending = false;
        qWarning() << "AudioOutput::could not start sndcpy:" << m_sndcpy.errorString();
        stopAudioOutput();
    });
    connect(&m_startupTimer, &QTimer::timeout, this, [this]() {
        if (!m_startPending) {
            return;
        }
        m_startPending = false;
        qWarning("AudioOutput::sndcpy setup timed out");
        if (QProcess::NotRunning != m_sndcpy.state()) {
            m_sndcpy.kill();
        }
        stopAudioOutput();
    });
    connect(this, &AudioOutput::audioDataReceived,
            this, &AudioOutput::writeAudioData, Qt::QueuedConnection);
}

AudioOutput::~AudioOutput()
{
    stop();
}

bool AudioOutput::start(const QString& serial, int port)
{
    if (m_running || m_startPending) {
        stop();
    }

    if (!startAudioOutput()) {
        return false;
    }

    m_pendingPort = port;
    m_startPending = true;
    m_startupTimer.start(45000);
    if (runSndcpyProcess(serial, port, false)) {
        return true;
    }
    m_startupTimer.stop();
    m_startPending = false;
    stopAudioOutput();
    return false;
}

void AudioOutput::stop()
{
    if (!m_running && !m_startPending && !m_workerThread.isRunning()
        && QProcess::NotRunning == m_sndcpy.state()) {
        return;
    }
    m_running = false;
    m_startPending = false;
    m_startupTimer.stop();

    stopRecvData();
    stopAudioOutput();
    if (QProcess::NotRunning != m_sndcpy.state()) {
        m_sndcpy.kill();
        m_sndcpy.waitForFinished(1000);
    }
}

void AudioOutput::installonly(const QString &serial, int port)
{
    runSndcpyProcess(serial, port, false);
}

bool AudioOutput::runSndcpyProcess(const QString &serial, int port, bool wait)
{
    if (QProcess::NotRunning != m_sndcpy.state()) {
        m_sndcpy.kill();
    }

#ifdef Q_OS_WIN32
    QStringList params{serial, QString::number(port)};
    m_sndcpy.setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_sndcpy.start(QDir(QCoreApplication::applicationDirPath()).filePath("sndcpy.bat"), params);
#else
    QStringList params{"sndcpy.sh", serial, QString::number(port)};
    m_sndcpy.setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_sndcpy.start("bash", params);
#endif

    if (wait && !m_sndcpy.waitForStarted(3000)) {
        qWarning() << "AudioOutput::start sndcpy process failed";
        return false;
    }
    if (wait && !m_sndcpy.waitForFinished()) {
        qWarning() << "AudioOutput::sndcpy process crashed";
        return false;
    }

    return true;
}

bool AudioOutput::startAudioOutput()
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    if (m_audioOutput) {
        return true;
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
        return false;
    }

    m_audioOutput = new QAudioOutput(format, this);
    connect(m_audioOutput, &QAudioOutput::stateChanged, this, [](QAudio::State state) {
        qInfo() << "AudioOutput::audio state changed:" << state;
    });
    m_audioOutput->setBufferSize(48000 * 2 * 2 * 80 / 1000);
    m_outputDevice = m_audioOutput->start();
    if (!m_outputDevice) {
        qWarning() << "AudioOutput::audio output device not available, cannot play audio.";
        delete m_audioOutput;
        m_audioOutput = nullptr;
        return false;
    }
    return true;
#else
    if (m_audioSink) {
        return true;
    }

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
    if (!defaultDevice.isFormatSupported(format)) {
        qWarning() << "AudioOutput::audio format not supported, cannot play audio.";
        return false;
    }
    m_audioSink = new QAudioSink(defaultDevice, format, this);
    m_audioSink->setBufferSize(48000 * 2 * 2 * 80 / 1000);
    m_outputDevice = m_audioSink->start();
    if (!m_outputDevice) {
        qWarning() << "AudioOutput::audio output device not available, cannot play audio.";
        delete m_audioSink;
        m_audioSink = nullptr;
        return false;
    }
    return true;
#endif
}

void AudioOutput::stopAudioOutput()
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
    });
    connect(audioSocket, &QIODevice::readyRead, audioSocket, [this, audioSocket]() {
        const QByteArray data = audioSocket->readAll();
        if (!data.isEmpty()) {
            emit audioDataReceived(data);
        }
    });
    connect(audioSocket, &QTcpSocket::stateChanged, audioSocket, [](QAbstractSocket::SocketState state) {
        qInfo() << "AudioOutput::audio socket state changed:" << state;
    });
    connect(audioSocket, &QTcpSocket::connected, audioSocket, [audioSocket]() {
        audioSocket->setProperty("retryCount", 0);
        qInfo("AudioOutput::audio socket connected");
    });
    const auto retryConnection = [audioSocket]() {
        const int retryCount = audioSocket->property("retryCount").toInt();
        if (retryCount >= 10) {
            qWarning("AudioOutput::audio socket reconnect limit reached");
            return;
        }
        audioSocket->setProperty("retryCount", retryCount + 1);
        QTimer::singleShot(300, audioSocket, [audioSocket]() {
            if (audioSocket->state() == QAbstractSocket::UnconnectedState) {
                audioSocket->connectToHost(QHostAddress::LocalHost,
                                           audioSocket->property("audioPort").toInt());
            }
        });
    };
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(audioSocket, &QTcpSocket::errorOccurred, audioSocket, [retryConnection](QAbstractSocket::SocketError error) {
        qInfo() << "AudioOutput::audio socket error occurred:" << error;
        retryConnection();
    });
#else
    connect(audioSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), audioSocket, [retryConnection](QAbstractSocket::SocketError error) {
        qInfo() << "AudioOutput::audio socket error occurred:" << error;
        retryConnection();
    });
#endif

    audioSocket->setProperty("audioPort", port);
    m_workerThread.start();
    emit connectTo(port);
}

void AudioOutput::writeAudioData(const QByteArray &data)
{
    if (!m_outputDevice || data.isEmpty()) {
        return;
    }
    if (m_outputDevice->write(data.constData(), data.size()) < 0) {
        qWarning() << "AudioOutput::audio output write failed:"
                   << m_outputDevice->errorString();
    }
}

void AudioOutput::stopRecvData()
{
    if (!m_workerThread.isRunning()) {
        return;
    }

    m_workerThread.quit();
    m_workerThread.wait();
}
