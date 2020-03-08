#ifndef VIDEOFORM_H
#define VIDEOFORM_H

#include <QWidget>
#include <QPointer>

namespace Ui {
class videoForm;
}

struct AVFrame;
class ToolForm;
class Device;
class FileHandler;
class QYUVOpenGLWidget;
class VideoForm : public QWidget
{
    Q_OBJECT
public:
    explicit VideoForm(bool skin = true, QWidget *parent = 0);
    ~VideoForm();

    void staysOnTop(bool top = true);
    void updateShowSize(const QSize &newSize);
    void updateRender(const AVFrame *frame);
    void setDevice(Device *device);
    QRect getGrabCursorRect();
    const QSize &frameSize();

public slots:
    void onSwitchFullScreen();

private:    
    void updateStyleSheet(bool vertical);
    QMargins getMargins(bool vertical);
    void initUI();
    
    void showToolForm(bool show = true);
    void moveCenter();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    void paintEvent(QPaintEvent *);
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);

    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    // ui
    Ui::videoForm *ui;    
    QPointer<ToolForm> m_toolForm;
    QPointer<QWidget> m_loadingWidget;
    QPointer<QYUVOpenGLWidget> m_videoWidget;

    //inside member
    QSize m_frameSize;
    QPoint m_dragPosition;
    float m_widthHeightRatio = 0.5f;
    bool m_skin = true;
    QPoint m_fullScreenBeforePos;

    //outside member
    QPointer<Device> m_device;
};

#endif // VIDEOFORM_H
