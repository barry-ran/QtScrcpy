#include "overlaypanel.h"

OverlayPanel::OverlayPanel(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

OverlayPanel::~OverlayPanel() {}

void OverlayPanel::setSerial(const QString &serial)
{
    m_serial = serial;
}
