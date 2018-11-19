#ifndef VIDEOFORM_H
#define VIDEOFORM_H

#include <QWidget>

#include "server.h"
#include "decoder.h"
#include "frames.h"
#include "inputconvertnormal.h"
#include "inputconvertgame.h"

namespace Ui {
class videoForm;
}

class VideoForm : public QWidget
{
    Q_OBJECT

public:
    explicit VideoForm(const QString& serial, QWidget *parent = 0);
    ~VideoForm();

private:
    void updateShowSize(const QSize &newSize);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    Ui::videoForm *ui;
    QSize frameSize;
    Server* m_server = Q_NULLPTR;
    Decoder m_decoder;
    Frames m_frames;
    //InputConvertNormal m_inputConvert;
    InputConvertGame m_inputConvert;
    QString m_serial = "";
};

#endif // VIDEOFORM_H
