#include "overlaybutton.h"

OverlayButton::OverlayButton(QWidget *parent) : QPushButton(parent)
{
    setFocusPolicy(Qt::NoFocus);
}

OverlayButton::~OverlayButton() {}
