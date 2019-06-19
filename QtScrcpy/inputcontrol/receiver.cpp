#include <QTcpSocket>
#include <QApplication>
#include <QClipboard>

#include "receiver.h"
#include "controller.h"
#include "deviceevent.h"

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
        DeviceEvent deviceEvent;
        qint32 consume = deviceEvent.deserialize(byteArray);
        if (0 >= consume) {
            break;
        }
        controlSocket->read(consume);
        processEvent(&deviceEvent);
    }
}

void Receiver::processEvent(DeviceEvent *deviceEvent)
{
    switch (deviceEvent->type()) {
    case DeviceEvent::DET_GET_CLIPBOARD:
    {
        QClipboard *board = QApplication::clipboard();
        QString text;
        deviceEvent->getClipboardEventData(text);
        board->setText(text);
        break;
    }
    default:
        break;
    }
}
