#ifndef KEYMAPSTUDIO_H
#define KEYMAPSTUDIO_H

#include <QWidget>
#include "uibase/magneticwidget.h"

class OverlayPanel;

class KeymapStudio : public MagneticWidget
{
    Q_OBJECT
public:
    explicit KeymapStudio(QWidget *parent = nullptr, OverlayPanel *overlayPanel = nullptr, QWidget *adsorbWidget = nullptr);
    ~KeymapStudio() override;

    void setSerial(const QString &serial);

private:
    QString m_serial;
    OverlayPanel *m_overlayPanel = nullptr;
};

#endif // KEYMAPSTUDIO_H
