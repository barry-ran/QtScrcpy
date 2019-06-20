#ifndef VIDEOFORM_H
#define VIDEOFORM_H

#include <QWidget>
#include <QPointer>
#include <QTime>

#include "server.h"
#include "stream.h"
#include "filehandler.h"
#include "controller.h"

namespace Ui {
class videoForm;
}

class ToolForm;
class Recorder;
class VideoBuffer;
class Decoder;
class VideoForm : public QWidget
{
    Q_OBJECT
public:
    explicit VideoForm(const QString& serial, quint16 maxSize = 720, quint32 bitRate = 8000000, const QString& fileName = "", bool closeScreen = false, QWidget *parent = 0);
    ~VideoForm();

    void switchFullScreen();    
    void staysOnTop(bool top = true);
    Controller* getController();

private:
    void updateShowSize(const QSize &newSize);
    void updateStyleSheet(bool vertical);
    void initUI();
    void initSignals();    
    void showToolForm(bool show = true);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    void paintEvent(QPaintEvent *);
    void showEvent(QShowEvent *event);

    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    // ui
    Ui::videoForm *ui;    
    QPointer<ToolForm> m_toolForm;
    QPointer<QWidget> m_loadingWidget;

    // server relevant
    Server* m_server = Q_NULLPTR;    
    VideoBuffer* m_vb = Q_NULLPTR;
    Decoder* m_decoder = Q_NULLPTR;
    Recorder* m_recorder = Q_NULLPTR;
    QPointer<Controller> m_controller;
    Stream m_stream;
    FileHandler m_fileHandler;

    // server params
    QString m_serial = "";
    quint16 m_maxSize = 720;
    quint32 m_bitRate = 8000000;

    // assist member
    QSize frameSize;
    QPoint m_dragPosition;
    float m_widthHeightRatio = 0.5f;
    bool m_closeScreen = false;
    QTime m_startTimeCount;
};

#endif // VIDEOFORM_H
