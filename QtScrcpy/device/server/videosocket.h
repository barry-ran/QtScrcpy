#ifndef VIDEOSOCKET_H
#define VIDEOSOCKET_H

#include <QEvent>
#include <QMutex>
#include <QTcpSocket>
#include <QWaitCondition>

class VideoSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit VideoSocket(QObject *parent = nullptr);
    virtual ~VideoSocket();

    qint32 subThreadRecvData(quint8 *buf, qint32 bufSize);

protected:
    bool event(QEvent *event);

protected slots:
    void onReadyRead();
    void quitNotify();

private:
    QMutex m_mutex;
    QWaitCondition m_recvDataCond;
    bool m_recvData = false;
    quint8 *m_buffer = Q_NULLPTR;
    qint32 m_bufferSize = 0;
    qint32 m_dataSize = 0;
    bool m_quit = false;
};

#endif // VIDEOSOCKET_H
