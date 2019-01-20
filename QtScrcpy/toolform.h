#ifndef TOOLFORM_H
#define TOOLFORM_H

#include <QWidget>

#include "magneticwidget.h"

namespace Ui {
class ToolForm;
}

class ToolForm : public MagneticWidget
{
    Q_OBJECT

public:
    explicit ToolForm(QWidget* adsorbWidget, AdsorbPositions adsorbPos);
    ~ToolForm();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    Ui::ToolForm *ui;
    QPoint m_dragPosition;
};

#endif // TOOLFORM_H
