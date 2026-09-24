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
    ToolForm* toolForm() const;

private:
    void onFrame(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
                 int linesizeY, int linesizeU, int linesizeV) override;
    // VideoToolbox Metal Ã¨Â·Â¯Ã¥Â¾â€žÃ¥Â¸Â§Ã¥â€ºÅ¾Ã¨Â°Æ’Ã¯Â¼Ë†Ã¤Â»â€¦ macOS arm64Ã¯Â¼â€°
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
    // Ã¨Å½Â·Ã¥Ââ€“Ã¥Â½â€œÃ¥â€°ÂÃ¨Â§â€ Ã©Â¢â€˜Ã¦Â¸Â²Ã¦Å¸â€œ widgetÃ¯Â¼Ë†OpenGL Ã¦Ë†â€“ Metal Ã¥Â®Â¹Ã¥â„¢Â¨Ã¯Â¼â€°
    // Ã¦ËœÂ¯Ã¥ÂÂ¦Ã¤Â½Â¿Ã§â€Â¨ Metal Ã¦Â¸Â²Ã¦Å¸â€œÃ¨Â·Â¯Ã¥Â¾â€ž
    bool isMetalMode() const;

    // ui
    Ui::videoForm *ui;
    QPointer<ToolForm> m_toolForm;
    QPointer<QWidget> m_loadingWidget;
    QPointer<QYUVOpenGLWidget> m_videoWidget;

    // Metal Ã¦Â¸Â²Ã¦Å¸â€œÃ¨Â·Â¯Ã¥Â¾â€žÃ¯Â¼Ë†Ã¤Â»â€¦ macOS arm64Ã¯Â¼â€°
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
    bool m_metalFirstFrame = true;  // Metal Ã©Â¦â€“Ã¦Â¬Â¡Ã¥Â¸Â§Ã¦Â â€¡Ã¨Â®Â°
    bool m_flexDisplay = false;
    bool m_preventAutoResize = false;
    QTimer m_flexResizeTimer;
    QSize m_pendingDisplaySize;

    QPointer<OverlayPanel> m_overlayPanel;

    //Whether to display the toolbar when connecting a device.
    bool show_toolbar = true;
};

#endif // VIDEOFORM_H

