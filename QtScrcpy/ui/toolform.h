#ifndef TOOLFORM_H
#define TOOLFORM_H

#include <QPointer>
#include <QWidget>

#include "../QtScrcpyCore/include/QtScrcpyCore.h"
#include "magneticwidget.h"

namespace Ui
{
    class ToolForm;
}

class QGridLayout;
class QPushButton;
class Device;
class ToolForm : public QWidget
{
    Q_OBJECT

public:
    explicit ToolForm(QWidget *adsorbWidget, MagneticWidget::AdsorbPositions adsorbPos);
    ~ToolForm();

    void setSerial(const QString& serial);
    void setHorizontal(bool horizontal);
    void setFullscreenMode(bool fullscreen);
    bool isHost();
    void setAudioState(bool active, bool starting);

signals:
    void audioToggleRequested();

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
    void on_expandSettingsBtn_clicked();
    void on_rotateBtn_clicked();
    void on_touchBtn_clicked();
    void on_cameraTorchBtn_clicked();
    void on_cameraZoomOutBtn_clicked();
    void on_cameraZoomInBtn_clicked();
    void on_groupControlBtn_clicked();
    void on_openScreenBtn_clicked();
    void on_clipboardBtn_clicked();

private:
    void initStyle();
    void togglePanel(int panel);
    void resetPanel();
    void updateGroupControl();
    void updateCameraMode();

private:
    Ui::ToolForm *ui;
    QGridLayout *m_compactGrid = nullptr;
    QList<QPushButton*> m_compactButtons;
    QList<QPushButton*> m_extraButtons;
    int m_openPanel = 0; // Last panel requested by this toolbar; Android supplies no state feedback.
    bool m_horizontal = false;
    bool m_fullscreen = false;
    bool m_audioActive=false,m_audioStarting=false;
    bool m_layoutReady=false;
    QPushButton *m_audioButton=nullptr;
    QPoint m_dragPosition;
    QString m_serial;
    bool m_showTouch = false;
    bool m_cameraTorch = false;
    bool m_isHost = false;
};

#endif // TOOLFORM_H
