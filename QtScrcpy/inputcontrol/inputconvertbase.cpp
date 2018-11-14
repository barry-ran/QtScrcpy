#include "inputconvertbase.h"

InputConvertBase::InputConvertBase()
{

}

InputConvertBase::~InputConvertBase()
{

}

void InputConvertBase::setDeviceSocket(DeviceSocket *deviceSocket)
{
    m_controller.setDeviceSocket(deviceSocket);
}

void InputConvertBase::sendControlEvent(ControlEvent *event)
{
    if (event) {
        m_controller.postControlEvent(event);
    }
}

