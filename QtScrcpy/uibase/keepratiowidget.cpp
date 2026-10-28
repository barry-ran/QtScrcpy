#include <QResizeEvent>
#include <cmath>

#include "keepratiowidget.h"

KeepRatioWidget::KeepRatioWidget(QWidget *parent) : QWidget(parent) {}

KeepRatioWidget::~KeepRatioWidget() {}

void KeepRatioWidget::setWidget(QWidget *w)
{
    if (!w) {
        return;
    }
    w->setParent(this);
    m_subWidget = w;
}

void KeepRatioWidget::setWidthHeightRatio(float widthHeightRatio)
{
    if (fabs(m_widthHeightRatio - widthHeightRatio) < 0.000001f) {
        return;
    }
    m_widthHeightRatio = widthHeightRatio;
    adjustSubWidget();
}

const QSize KeepRatioWidget::goodSize()
{
    if (!m_subWidget || m_widthHeightRatio < 0.0f) {
        return QSize();
    }
    return m_subWidget->size();
}

void KeepRatioWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    adjustSubWidget();
}

void KeepRatioWidget::adjustSubWidget()
{
    if (!m_subWidget) {
        return;
    }

    QSize curSize = size();
    QPoint pos(0, 0);
    int width = 0;
    int height = 0;
    if(m_widthHeightRatio>0.0f){
        if(curSize.width()/m_widthHeightRatio>curSize.height()){
            height=curSize.height();width=qRound(height*m_widthHeightRatio);
        }else{width=curSize.width();height=qRound(width/m_widthHeightRatio);}
        // Fit both axes; never allow the rendered child to overflow its host.
        if(width>curSize.width()){width=curSize.width();height=qRound(width/m_widthHeightRatio);}
        if(height>curSize.height()){height=curSize.height();width=qRound(height*m_widthHeightRatio);}
        pos=QPoint((curSize.width()-width)/2,(curSize.height()-height)/2);
    }else{width=curSize.width();height=curSize.height();}
    m_subWidget->setGeometry(pos.x(), pos.y(), width, height);
}
