#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include "server.h"

namespace Ui {
class Dialog;
}

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
};

#endif // DIALOG_H
