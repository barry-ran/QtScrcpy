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
    void toggleKeymapEdit(); ///< called by ToolForm keymapBtn
    bool isHost();
    QWidget* videoWidget() const;
    ToolForm* toolForm() const { return m_toolForm; }

private:
    void onFrame(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
                 int linesizeY, int linesizeU, int linesizeV) override;
    // VideoToolbox Metal è·¯å¾„å¸§å›žè°ƒï¼ˆä»… macOS arm64ï¼‰
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

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    // èŽ·å–å½“å‰è§†é¢‘æ¸²æŸ“ widgetï¼ˆOpenGL æˆ– Metal å®¹å™¨ï¼‰
    // æ˜¯å¦ä½¿ç”¨ Metal æ¸²æŸ“è·¯å¾„
    bool isMetalMode() const;

    // ui
    Ui::videoForm *ui;
    QPointer<ToolForm> m_toolForm;
    QPointer<QWidget> m_loadingWidget;
    QPointer<QYUVOpenGLWidget> m_videoWidget;

    // Metal æ¸²æŸ“è·¯å¾„ï¼ˆä»… macOS arm64ï¼‰
    QPointer<MetalVideoWidget> m_metalWidget;

    QPointer<QLabel> m_fpsLabel;

    //inside member
    QSize m_frameSize;
    QSize m_normalSize;
    QPoint m_dragPosition;
    float m_widthHeightRatio = 0.5f;
    bool m_skin = true;
    QPoint m_fullScreenBeforePos;
    QString m_serial;
    int m_decodeMode = 0;
    bool m_metalFirstFrame = true;  // Metal é¦–æ¬¡å¸§æ ‡è®°
    bool m_flexDisplay = false;
    bool m_preventAutoResize = false;
    QTimer m_flexResizeTimer;
    QSize m_pendingDisplaySize;

    QPointer<OverlayPanel> m_overlayPanel;

    //Whether to display the toolbar when connecting a device.
    bool show_toolbar = true;
};

#endif // VIDEOFORM_H

