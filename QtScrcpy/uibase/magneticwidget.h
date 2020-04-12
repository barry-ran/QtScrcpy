#ifndef MAGNETICWIDGET_H
#define MAGNETICWIDGET_H

#include <QPointer>
#include <QWidget>

/*
 * a magnetic widget
 * window title bar support not good
*/

class MagneticWidget : public QWidget
{
    Q_OBJECT

public:
    enum AdsorbPosition
    {
        AP_OUTSIDE_LEFT = 0x01,   // 吸附外部左边框
        AP_OUTSIDE_TOP = 0x02,    // 吸附外部上边框
        AP_OUTSIDE_RIGHT = 0x04,  // 吸附外部右边框
        AP_OUTSIDE_BOTTOM = 0x08, // 吸附外部下边框
        AP_INSIDE_LEFT = 0x10,    // 吸附内部左边框
        AP_INSIDE_TOP = 0x20,     // 吸附内部上边框
        AP_INSIDE_RIGHT = 0x40,   // 吸附内部右边框
        AP_INSIDE_BOTTOM = 0x80,  // 吸附内部下边框
        AP_ALL = 0xFF,            // 全吸附
    };
    Q_DECLARE_FLAGS(AdsorbPositions, AdsorbPosition)

public:
    explicit MagneticWidget(QWidget *adsorbWidget, AdsorbPositions adsorbPos = AP_ALL);
    ~MagneticWidget();

    bool isAdsorbed();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private:
    void getGeometry(QRect &relativeWidgetRect, QRect &targetWidgetRect);

private:
    AdsorbPositions m_adsorbPos = AP_ALL;
    QPoint m_relativePos;
    bool m_adsorbed = false;
    QPointer<QWidget> m_adsorbWidget;
    AdsorbPosition m_curAdsorbPosition;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MagneticWidget::AdsorbPositions)
#endif // MAGNETICWIDGET_H
