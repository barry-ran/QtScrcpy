#ifndef VIDEOFORM_H
#define VIDEOFORM_H

#include <QPointer>
#include <QTimer>
#include <QWidget>

#include "../QtScrcpyCore/include/QtScrcpyCore.h"

namespace Ui
{
    class videoForm;
}

class ToolForm;
class OverlayPanel;
class FileHandler;
class QYUVOpenGLWidget;
class QLabel;
class MetalVideoWidget;
class DeviceInfoOverlay;
class RecoilAssist;
class TurboMode;
class GamepadManager;

class VideoForm : public QWidget, public qsc::DeviceObserver
{
    Q_OBJECT
public:
    explicit VideoForm(bool framelessWindow = false, bool skin = true, bool showToolBar = true, int decodeMode = 0, QWidget *parent = 0);
    ~VideoForm();

    void staysOnTop(bool top = true);
    void updateShowSize(const QSize &newSize);
    void updateRender(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV, int linesizeY, int linesizeU, int linesizeV);
    void setSerial(const QString& serial);
    QRect getGrabCursorRect();
    const QSize &frameSize();
    void resizeSquare();
    void removeBlackRect();
    void showFPS(bool show);
    void switchFullScreen();
    void toggleKeymapEdit();    ///< called by ToolForm keymapBtn
    void toggleTurboMode();
    void toggleGamepad();     ///< Turbo/Burst mode
    void toggleDeviceInfo();    ///< Battery + Temp overlay
    bool isHost();
    QWidget* videoWidget() const;
    ToolForm* toolForm() const;

private:
    void onFrame(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
                 int linesizeY, int linesizeU, int linesizeV) override;
    void onFrameMetal(void* cvPixelBuffer, int width, int height) override;
    void updateFPS(quint32 fps) override;
    void onVideoSessionChanged(const QSize &size, bool clientResized) override;
    void grabCursor(bool grab) override;

    void updateStyleSheet(bool vertical);
    QMargins getMargins(bool vertical);
    void initUI();

    void showToolForm(bool show = true);
    void moveCenter();
    void installShortcut();
    QRect getScreenRect();

    // Recoil assist: apply WinAPI relative mouse move
    void applyRecoilCompensation(int dx, int dy);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    bool isMetalMode() const;

    // UI
    Ui::videoForm *ui;
    QPointer<ToolForm>          m_toolForm;
    QPointer<QWidget>           m_loadingWidget;
    QPointer<QYUVOpenGLWidget>  m_videoWidget;
    QPointer<MetalVideoWidget>  m_metalWidget;
    QPointer<QLabel>            m_fpsLabel;

    // Inside member
    QSize   m_frameSize;
    QSize   m_normalSize;
    QPoint  m_dragPosition;
    float   m_widthHeightRatio = 0.5f;
    bool    m_skin             = true;
    QPoint  m_fullScreenBeforePos;
    QString m_serial;
    int     m_decodeMode       = 0;
    bool    m_metalFirstFrame  = true;
    bool    m_flexDisplay      = false;
    bool    m_preventAutoResize = false;
    QTimer  m_flexResizeTimer;
    QSize   m_pendingDisplaySize;

    // Overlay: separate top-level window to avoid blocking OpenGL
    QPointer<OverlayPanel>      m_overlayPanel;

    // New features
    QPointer<DeviceInfoOverlay> m_deviceInfoOverlay;
    QPointer<RecoilAssist>      m_recoilAssist;
    QPointer<TurboMode>         m_turboMode;
    QPointer<GamepadManager>    m_gamepadManager;

    // Whether to display the toolbar when connecting a device.
    bool show_toolbar = true;
};

#endif // VIDEOFORM_H
