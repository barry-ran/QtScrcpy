#ifndef VIDEOFORM_H
#define VIDEOFORM_H

#include <QWidget>

#include "server.h"
#include "decoder.h"
#include "frames.h"

namespace Ui {
class videoForm;
}

class VideoForm : public QWidget
{
    Q_OBJECT

public:
    explicit VideoForm(QWidget *parent = 0);
    ~VideoForm();

private:
    void updateShowSize(const QSize &newSize);

private:
    Ui::videoForm *ui;
    Server* m_server = Q_NULLPTR;
    Decoder m_decoder;
    Frames m_frames;
};

#endif // VIDEOFORM_H
