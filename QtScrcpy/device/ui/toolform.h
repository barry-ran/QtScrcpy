#ifndef TOOLFORM_H
#define TOOLFORM_H

#include <QPointer>
#include <QWidget>

#include "device.h"
#include "magneticwidget.h"

namespace Ui
{
    class ToolForm;
}

class Device;
class ToolForm : public MagneticWidget
{
    Q_OBJECT

public:
    explicit ToolForm(QWidget *adsorbWidget, AdsorbPositions adsorbPos);
    ~ToolForm();

    void setDevice(Device *device);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void on_fullScreenBtn_clicked();
    void on_returnBtn_clicked();
    void on_homeBtn_clicked();
    void on_menuBtn_clicked();
    void on_appSwitchBtn_clicked();
    void on_powerBtn_clicked();
    void on_screenShotBtn_clicked();
    void on_volumeUpBtn_clicked();
    void on_volumeDownBtn_clicked();
    void on_closeScreenBtn_clicked();
    void on_expandNotifyBtn_clicked();
    void on_touchBtn_clicked();
    void on_groupControlBtn_clicked();

    void onControlStateChange(Device *device, Device::GroupControlState oldState, Device::GroupControlState newState);

private:
    void initStyle();
    void updateGroupControl();

private:
    Ui::ToolForm *ui;
    QPoint m_dragPosition;
    QPointer<Device> m_device;
    bool m_showTouch = false;
};

#endif // TOOLFORM_H
