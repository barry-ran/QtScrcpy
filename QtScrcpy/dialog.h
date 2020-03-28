#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QPointer>

#include "adbprocess.h"
#include "devicemanage.h"

namespace Ui
{
    class Dialog;
}

class QYUVOpenGLWidget;
class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    void outLog(const QString &log, bool newLine = true);
    bool filterLog(const QString &log);
    void getIPbyIp();

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

    void on_adbCommandBtn_clicked();

    void on_stopAdbBtn_clicked();

    void on_clearOut_clicked();

    void on_stopAllServerBtn_clicked();

    void on_refreshGameScriptBtn_clicked();

    void on_applyScriptBtn_clicked();

    void on_recordScreenCheck_clicked(bool checked);

    void on_bitRateBox_activated(int index);

    void on_maxSizeBox_activated(int index);

    void on_formatBox_activated(int index);

    void on_framelessCheck_stateChanged(int arg1);

private:
    bool checkAdbRun();
    void initUI();
    void execAdbCmd();
    QString getGameScript(const QString &fileName);

private:
    Ui::Dialog *ui;
    AdbProcess m_adb;
    DeviceManage m_deviceManage;
};

#endif // DIALOG_H
