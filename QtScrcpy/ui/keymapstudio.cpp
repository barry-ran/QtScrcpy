#include "keymapstudio.h"
#include "overlaypanel.h"
#include <QVBoxLayout>
#include <QLabel>

KeymapStudio::KeymapStudio(QWidget *parent, OverlayPanel *overlayPanel, QWidget *adsorbWidget)
    : MagneticWidget(adsorbWidget ? adsorbWidget : parent, AP_ALL)
    , m_overlayPanel(overlayPanel)
{
    setWindowTitle(tr("Keymap Studio"));
    resize(300, 400);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Keymap Studio Editor"), this));
}

KeymapStudio::~KeymapStudio() {}

void KeymapStudio::setSerial(const QString &serial)
{
    m_serial = serial;
}
