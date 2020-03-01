#include <QResizeEvent>

#include "keepradiowidget.h"

KeepRadioWidget::KeepRadioWidget(QWidget *parent) :
    QWidget(parent)
{

}

KeepRadioWidget::~KeepRadioWidget()
{

}

void KeepRadioWidget::setWidget(QWidget *w)
{
    if (!w) {
        return;
    }
    w->setParent(this);
    m_subWidget = w;
}

void KeepRadioWidget::setWidthHeightRadio(float widthHeightRadio)
{
    if (fabs(m_widthHeightRadio - widthHeightRadio) < 0.000001f) {
        return;
    }
    m_widthHeightRadio = widthHeightRadio;
    adjustSubWidget();
}

const QSize KeepRadioWidget::goodSize()
{
    if (!m_subWidget || m_widthHeightRadio < 0.0f) {
        return QSize();
    }
    return m_subWidget->size();
}

void KeepRadioWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    adjustSubWidget();
}

void KeepRadioWidget::adjustSubWidget()
{
    if (!m_subWidget) {
        return;
    }

    QSize curSize = size();
    QPoint pos(0, 0);
    int width = 0;
    int height = 0;
    if (m_widthHeightRadio > 1.0f) {
        // base width
        width = curSize.width();
        height = curSize.width() / m_widthHeightRadio;
        pos.setY((curSize.height() - height) / 2);
    } else if (m_widthHeightRadio > 0.0f) {
        // base height
        height = curSize.height();
        width = curSize.height() * m_widthHeightRadio;
        pos.setX((curSize.width() - width) / 2);
    } else {
        // full widget
        height = curSize.height();
        width = curSize.width();
    }
    m_subWidget->setGeometry(pos.x(), pos.y(), width, height);
}
