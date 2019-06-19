#include <QTcpSocket>
#include <QApplication>
#include <QClipboard>

#include "receiver.h"
#include "controller.h"
#include "devicemsg.h"

Receiver::Receiver(Controller* controller) : QObject(controller)
{
    m_controller = controller;
    Q_ASSERT(controller);
}

Receiver::~Receiver()
{

}

void Receiver::onReadyRead()
{
    QTcpSocket* controlSocket = m_controller->getControlSocket();
    if (!controlSocket) {
        return;
    }

    while (controlSocket->bytesAvailable()) {
        QByteArray byteArray = controlSocket->peek(controlSocket->bytesAvailable());
        DeviceMsg deviceMsg;
        qint32 consume = deviceMsg.deserialize(byteArray);
        if (0 >= consume) {
            break;
        }
        controlSocket->read(consume);
        processMsg(&deviceMsg);
    }
}

void Receiver::processMsg(DeviceMsg *deviceMsg)
{
    switch (deviceMsg->type()) {
    case DeviceMsg::DMT_GET_CLIPBOARD:
    {
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
