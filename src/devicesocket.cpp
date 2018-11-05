#include "devicesocket.h"

DeviceSocket::DeviceSocket(QObject *parent) : QTcpSocket(parent)
{
    connect(this, &DeviceSocket::readyRead, this, &DeviceSocket::onReadyRead);
}

DeviceSocket::~DeviceSocket()
{

}

qint32 DeviceSocket::recvData(quint8 *buf, qint32 bufSize)
{
    QMutexLocker locker(&m_mutex);

    m_buffer = buf;
    m_bufferSize = bufSize;
    // post event
    while (!m_recvData) {
        m_recvDataCond.wait(&m_mutex);
    }
    return m_dataSize;
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
