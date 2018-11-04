#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QPointer>

#include "videoform.h"

namespace Ui {
class Dialog;
}

class QYUVOpenGLWidget;
class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private slots:
    void on_adbProcess_clicked();

    void on_startServerBtn_clicked();

    void on_stopServerBtn_clicked();

private:
    Ui::Dialog *ui;

    QPointer<VideoForm> m_videoForm;
};

#endif // DIALOG_H
