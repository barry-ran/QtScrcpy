#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QPointer>

#include "adbprocess.h"

namespace Ui {
class Dialog;
}

class Device;
class QYUVOpenGLWidget;
class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    void outLog(const QString& log, bool newLine = true);

private slots:
    void on_updateDevice_clicked();

    void on_startServerBtn_clicked();

    void on_stopServerBtn_clicked();

    void on_wirelessConnectBtn_clicked();

    void on_startAdbdBtn_clicked();

    void on_getIPBtn_clicked();

    void on_wirelessDisConnectBtn_clicked();

    void on_selectRecordPathBtn_clicked();

    void on_recordPathEdt_textChanged(const QString &arg1);

    void on_alwaysTopCheck_stateChanged(int arg1);

    void on_closeScreenCheck_stateChanged(int arg1);

    void on_adbCommandBtn_clicked();

    void on_stopAdbBtn_clicked();

    void on_clearOut_clicked();

private:
    bool checkAdbRun();
    void initUI();
    void execAdbCmd();

private:
    Ui::Dialog *ui;
    AdbProcess m_adb;
    QPointer<Device> m_device;
};

#endif // DIALOG_H
