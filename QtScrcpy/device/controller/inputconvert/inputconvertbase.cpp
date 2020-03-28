#include "inputconvertbase.h"
#include "controller.h"

InputConvertBase::InputConvertBase(Controller *controller) : QObject(controller), m_controller(controller)
{
    Q_ASSERT(controller);
}

InputConvertBase::~InputConvertBase() {}

void InputConvertBase::sendControlMsg(ControlMsg *msg)
{
    if (msg && m_controller) {
        m_controller->postControlMsg(msg);
    }
}
