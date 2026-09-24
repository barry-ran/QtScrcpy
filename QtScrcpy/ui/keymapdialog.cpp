#include "keymapdialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

KeymapDialog::KeymapDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Keymap Settings"));
    resize(400, 300);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Configure key mapping for device."), this));
    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(btnBox);
}

KeymapDialog::~KeymapDialog() {}

void KeymapDialog::setSerial(const QString &serial)
{
    m_serial = serial;
}
