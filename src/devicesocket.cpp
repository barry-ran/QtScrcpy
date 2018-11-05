#include <QCoreApplication>
#include <QDebug>

#include "devicesocket.h"

static const int GetDataEvent = QEvent::registerEventType(QEvent::User+1);
class DeviceSocketEvent : public QEvent
{
public:
    DeviceSocketEvent() : QEvent(QEvent::Type(GetDataEvent)){}
};

DeviceSocket::DeviceSocket(QObject *parent) : QTcpSocket(parent)
{
    connect(this, &DeviceSocket::readyRead, this, &DeviceSocket::onReadyRead);
    connect(this, &DeviceSocket::aboutToClose, this, &DeviceSocket::quitNotify);
    connect(this, &DeviceSocket::disconnected, this, &DeviceSocket::quitNotify);

    installEventFilter(this);
}

DeviceSocket::~DeviceSocket()
{
    quitNotify();
}

qint32 DeviceSocket::recvData(quint8 *buf, qint32 bufSize)
{
    QMutexLocker locker(&m_mutex);

    m_buffer = buf;
    m_bufferSize = bufSize;
    m_dataSize = 0;

    // post event
    DeviceSocketEvent* getDataEvent = new DeviceSocketEvent();
    QCoreApplication::postEvent(this, getDataEvent);

    // wait
    while (!m_recvData) {
        m_recvDataCond.wait(&m_mutex);
    }

    m_recvData = false;
    return m_dataSize;
}

bool DeviceSocket::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == GetDataEvent) {
        onReadyRead();
        return true;
    }
    return false;
}

void DeviceSocket::onReadyRead()
{
    QMutexLocker locker(&m_mutex);
    if (m_buffer && 0 < bytesAvailable()) {
        // recv data
        qint64 readSize = qMin(bytesAvailable(), (qint64)m_bufferSize);
        m_dataSize = read((char*)m_buffer, readSize);

        m_buffer = Q_NULLPTR;
        m_bufferSize = 0;
        m_recvData = true;
        m_recvDataCond.wakeOne();
    }
}

void DeviceSocket::quitNotify()
{
    QMutexLocker locker(&m_mutex);
    if (m_buffer) {
        m_buffer = Q_NULLPTR;
        m_bufferSize = 0;
        m_recvData = true;
        m_dataSize = 0;
        m_recvDataCond.wakeOne();
    }
}
