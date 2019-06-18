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

void InputConvertBase::sendControlEvent(ControlEvent *event)
{
    if (event) {
        m_controller.postControlEvent(event);
    }
}

