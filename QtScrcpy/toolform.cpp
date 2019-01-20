#include <QMouseEvent>

#include "toolform.h"
#include "ui_toolform.h"

ToolForm::ToolForm(QWidget* adsorbWidget, AdsorbPositions adsorbPos)
    : MagneticWidget(adsorbWidget, adsorbPos)
    , ui(new Ui::ToolForm)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
}

ToolForm::~ToolForm()
{
    delete ui;
}

void ToolForm::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void ToolForm::mouseReleaseEvent(QMouseEvent *event)
{

}

void ToolForm::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}
