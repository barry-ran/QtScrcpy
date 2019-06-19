#include "inputconvertbase.h"

InputConvertBase::InputConvertBase()
{

}

InputConvertBase::~InputConvertBase()
{

}

void InputConvertBase::setControlSocket(QTcpSocket *controlSocket)
{
    m_controller.setControlSocket(controlSocket);
}

void InputConvertBase::sendControlMsg(ControlMsg *msg)
{
    if (msg) {
        m_controller.postControlMsg(msg);
    }
}

