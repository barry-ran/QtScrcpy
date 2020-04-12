#include <QCoreApplication>
#include <QDebug>
#include <QThread>

#include "qscrcpyevent.h"
#include "videosocket.h"

VideoSocket::VideoSocket(QObject *parent) : QTcpSocket(parent)
{
    connect(this, &VideoSocket::readyRead, this, &VideoSocket::onReadyRead);
    connect(this, &VideoSocket::aboutToClose, this, &VideoSocket::quitNotify);
    connect(this, &VideoSocket::disconnected, this, &VideoSocket::quitNotify);
}

VideoSocket::~VideoSocket()
{
    quitNotify();
}

qint32 VideoSocket::subThreadRecvData(quint8 *buf, qint32 bufSize)
{
    // this function cant call in main thread
    Q_ASSERT(QCoreApplication::instance()->thread() != QThread::currentThread());
    if (m_quit) {
        return 0;
    }
    QMutexLocker locker(&m_mutex);

    m_buffer = buf;
    m_bufferSize = bufSize;
    m_dataSize = 0;

    // post event
    VideoSocketEvent *getDataEvent = new VideoSocketEvent();
    QCoreApplication::postEvent(this, getDataEvent);

    // wait
    while (!m_recvData) {
        m_recvDataCond.wait(&m_mutex);
    }

    m_recvData = false;
    return m_dataSize;
}

bool VideoSocket::event(QEvent *event)
{
    if (static_cast<QScrcpyEvent::Type>(event->type()) == QScrcpyEvent::VideoSocket) {
        onReadyRead();
        return true;
    }
    return QTcpSocket::event(event);
}

void VideoSocket::onReadyRead()
{
    QMutexLocker locker(&m_mutex);
    if (m_buffer && m_bufferSize <= bytesAvailable()) {
        // recv data
        qint64 readSize = qMin(bytesAvailable(), (qint64)m_bufferSize);
        m_dataSize = read((char *)m_buffer, readSize);

        m_buffer = Q_NULLPTR;
        m_bufferSize = 0;
        m_recvData = true;
        m_recvDataCond.wakeOne();
    }
}

void VideoSocket::quitNotify()
{
    m_quit = true;
    QMutexLocker locker(&m_mutex);
    if (m_buffer) {
        m_buffer = Q_NULLPTR;
        m_bufferSize = 0;
        m_recvData = true;
        m_dataSize = 0;
        m_recvDataCond.wakeOne();
    }
}
