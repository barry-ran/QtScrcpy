#ifndef OVERLAYPANEL_H
#define OVERLAYPANEL_H

#include <QWidget>

class OverlayPanel : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayPanel(QWidget *parent = nullptr);
    ~OverlayPanel() override;

    void setSerial(const QString &serial);

private:
    QString m_serial;
};

#endif // OVERLAYPANEL_H
