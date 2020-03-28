#include <QApplication>
#include <QClipboard>
#include <QTcpSocket>

#include "devicemsg.h"
#include "receiver.h"

Receiver::Receiver(QObject *parent) : QObject(parent) {}

Receiver::~Receiver() {}

void Receiver::setControlSocket(QTcpSocket *controlSocket)
{
    if (m_controlSocket || !controlSocket) {
        return;
    }
    m_controlSocket = controlSocket;
    connect(controlSocket, &QTcpSocket::readyRead, this, &Receiver::onReadyRead);
}

void Receiver::onReadyRead()
{
    if (!m_controlSocket) {
        return;
    }

    while (m_controlSocket->bytesAvailable()) {
        QByteArray byteArray = m_controlSocket->peek(m_controlSocket->bytesAvailable());
        DeviceMsg deviceMsg;
        qint32 consume = deviceMsg.deserialize(byteArray);
        if (0 >= consume) {
            break;
        }
        m_controlSocket->read(consume);
        processMsg(&deviceMsg);
    }
}

void Receiver::processMsg(DeviceMsg *deviceMsg)
{
    switch (deviceMsg->type()) {
    case DeviceMsg::DMT_GET_CLIPBOARD: {
        qInfo("Device clipboard copied");
        QClipboard *board = QApplication::clipboard();
        QString text;
        deviceMsg->getClipboardMsgData(text);
        board->setText(text);
        break;
    }
    default:
        break;
    }
}
