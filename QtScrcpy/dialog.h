#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QPointer>

#include "videoform.h"
#include "adbprocess.h"

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
    void on_updateDevice_clicked();

    void on_startServerBtn_clicked();

    void on_stopServerBtn_clicked();

    void on_wirelessConnectBtn_clicked();

    void on_startAdbdBtn_clicked();

    void on_getIPBtn_clicked();

private:
    void outLog(const QString& log);
    bool checkAdbRun();

private:
    Ui::Dialog *ui;
    AdbProcess m_adb;
    QPointer<VideoForm> m_videoForm;
};

#endif // DIALOG_H
