#ifndef OVERLAYBUTTON_H
#define OVERLAYBUTTON_H

#include <QPushButton>

class OverlayButton : public QPushButton
{
    Q_OBJECT
public:
    explicit OverlayButton(QWidget *parent = nullptr);
    ~OverlayButton() override;
};

#endif // OVERLAYBUTTON_H
