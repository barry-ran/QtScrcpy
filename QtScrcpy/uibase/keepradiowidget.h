#ifndef KEEPRADIOWIDGET_H
#define KEEPRADIOWIDGET_H

#include <QWidget>
#include <QPointer>

class KeepRadioWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KeepRadioWidget(QWidget *parent = nullptr);
    ~KeepRadioWidget();

    void setWidget(QWidget* w);
    void setWidthHeightRadio(float widthHeightRadio);
    const QSize goodSize();

protected:
    void resizeEvent(QResizeEvent *event);
    void adjustSubWidget();

private:
    float m_widthHeightRadio = -1.0f;
    QPointer<QWidget> m_subWidget;
    QSize m_goodSize;

};

#endif // KEEPRADIOWIDGET_H
