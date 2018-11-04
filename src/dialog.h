#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include "server.h"
#include "decoder.h"
#include "frames.h"

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
    Server* server;
    Decoder decoder;
    Frames frames;
    QYUVOpenGLWidget* w;
};

#endif // DIALOG_H
