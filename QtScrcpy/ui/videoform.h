#ifndef VIDEOFORM_H
#define VIDEOFORM_H

#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QWidget>
#include <QSet>

#include "../QtScrcpyCore/include/QtScrcpyCore.h"

namespace Ui
{
    class videoForm;
}

class ToolForm;
class FileHandler;
class QYUVOpenGLWidget;
class QLabel;
class QSizeGrip;
class MetalVideoWidget;
class QProcess;
class QPushButton;
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
    void toggleExpanded();
    void setAudioState(bool active,bool starting);

signals:
    void audioToggleRequested();
    void sessionCloseRequested(const QString &serial);

public:
    void restoreExpandedForDrag(const QPoint &global);
    bool isHost();
    void ensureOnScreen();
    void showProjectionControls();
    void hideProjectionControls();
    void setChromeDragging(bool dragging);
    void minimizeProjection();
    void finishSession();
    QRect phoneRect() const;
    QSize windowSizeForPhone(const QSize &size) const;
    QSize phoneSizeForWindow(const QSize &size) const;

private:
    void onFrame(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV,
                 int linesizeY, int linesizeU, int linesizeV) override;
    // VideoToolbox Metal 路径帧回调（仅 macOS arm64）
    void onFrameMetal(void* cvPixelBuffer, int width, int height) override;
    void updateFPS(quint32 fps) override;
    void onVideoSessionChanged(const QSize &size, bool clientResized) override;
    void grabCursor(bool grab) override;

    void updateStyleSheet(bool vertical);
    QMargins getMargins(bool vertical);
    void initUI();
    bool desktopImeEnabled() const;
    QString m_imePreedit;
    QPoint m_imeAnchor = QPoint(20, 40);
    QSet<int> m_forwardedKeys;

    void showToolForm(bool show = true);
    void moveCenter();
    void installShortcut();
    QRect getScreenRect();
    // 窗口越界回收：显示器插拔后，若窗口落在任何屏幕的可用区域之外，
    // 将其移回主屏居中，避免成为看不见的"幽灵窗口"。

    // 是否与任一屏幕的可用区域有交集（含边界相交判定）
    bool isRectOnAnyScreen(const QRect &rect) const;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    bool eventFilter(QObject *watched,QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    // 获取当前视频渲染 widget（OpenGL 或 Metal 容器）
    QWidget* videoWidget() const;
    void createProjectionChrome();
    void placeProjectionChrome();
    void placeToolForm();
    QMargins projectionInsets() const;
    void updateProjectionMask();
    void saveWindowGeometry();
    // 是否使用 Metal 渲染路径
    bool isMetalMode() const;

    // ui
    Ui::videoForm *ui;
    QPointer<ToolForm> m_toolForm;
    QPointer<QWidget> m_titleChrome;
    QPointer<QLabel> m_chromeTitle;
    QPointer<QSizeGrip> m_resizeGrip;
    QPointer<QWidget> m_topLeftGrip;
    bool m_chromeDragging=false;
    bool m_projectionMinimizing=false;
    bool m_sessionEnding=false;
    QTimer m_toolVisibilityTimer;
    QElapsedTimer m_toolIdleTime;
    QPoint m_lastToolCursor;
    bool m_toolsOnRight = false;
    bool m_audioActive=false,m_audioStarting=false;
    bool m_cursorGrabbed = false;
    QPointer<QWidget> m_loadingWidget;
    QPointer<QYUVOpenGLWidget> m_videoWidget;

    // Metal 渲染路径（仅 macOS arm64）
    QPointer<MetalVideoWidget> m_metalWidget;

    QPointer<QLabel> m_fpsLabel;

    //inside member
    QSize m_frameSize;
    QSize m_normalSize;
    QTimer m_aspectResizeTimer;
    QSize m_pendingAspectSize;
    bool m_adjustingAspect=false;
    bool m_fullscreenTransition=false;
    bool m_pendingFullscreenToggle=false;
    int m_fullscreenTransitionId=0;
    void forwardHeldPhonePress();
    QTimer m_holdToMove;
    bool m_pendingPhonePress=false;
    bool m_windowDragActive=false;
    QPointF m_holdLocal,m_holdGlobal;
    QPoint m_dragPosition;
    float m_widthHeightRatio = 0.5f;
    bool m_skin = true;
    QPoint m_fullScreenBeforePos;
    QString m_serial;
    int m_decodeMode = 0;
    bool m_metalFirstFrame = true;  // Metal 首次帧标记
    bool m_flexDisplay = false;
    bool m_preventAutoResize = false;
    QTimer m_flexResizeTimer;
    QSize m_pendingDisplaySize;

    //Whether to display the toolbar when connecting a device.
    bool show_toolbar = true;
};

#endif // VIDEOFORM_H
