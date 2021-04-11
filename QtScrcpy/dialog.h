﻿#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QPointer>
#include <QMessageBox>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QListWidget>


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

    void on_usbConnectBtn_clicked();

    void on_wifiConnectBtn_clicked();

    void on_connectedPhoneList_itemDoubleClicked(QListWidgetItem *item);

    void on_updateNameBtn_clicked();

    void on_useSingleModeCheck_clicked();

private:
    bool checkAdbRun();
    void initUI();
    void execAdbCmd();
    void delayMs(int ms);
    QString getGameScript(const QString &fileName);
    void slotShow();
    void slotActivated(QSystemTrayIcon::ActivationReason reason);
    void updateConnectedList();
    void updateUser();
    void loadUser();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::Dialog *ui;
    AdbProcess m_adb;
    DeviceManage m_deviceManage;
    QSystemTrayIcon *m_hideIcon;
    QMenu *m_menu;
    QAction *m_showWindow;
    QAction *m_quit;
};

#endif // DIALOG_H
