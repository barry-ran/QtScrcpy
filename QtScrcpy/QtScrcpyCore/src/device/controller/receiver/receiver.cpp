#include <QApplication>
#include <QClipboard>

#include "devicemsg.h"
#include "receiver.h"

Receiver::Receiver(QObject *parent) : QObject(parent) {}

Receiver::~Receiver() {}

void Receiver::recvDeviceMsg(DeviceMsg *deviceMsg)
{
    switch (deviceMsg->type()) {
    case DeviceMsg::DMT_GET_CLIPBOARD: {
        qInfo("Device clipboard copied");
        QClipboard *board = QApplication::clipboard();
        QString text;
        deviceMsg->getClipboardMsgData(text);

        if (board->text() == text) {
            qDebug("Computer clipboard unchanged");
            break;
        }
        board->setText(text);
        break;
    }
    default:
        break;
    }
}
