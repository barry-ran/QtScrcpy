#ifndef DIALOG_H
#define DIALOG_H

#include <QWidget>
#include <QPointer>
#include <QMessageBox>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QListWidget>
#include <QTimer>


#include "adbprocess.h"
#include "../QtScrcpyCore/include/QtScrcpyCore.h"
#include "audio/audiooutput.h"
#include "mtkconfigdialog.h"

namespace Ui
{
    class Widget;
}

class QYUVOpenGLWidget;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QPushButton;
class Dialog : public QWidget
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    void outLog(const QString &log, bool newLine = true);
    bool filterLog(const QString &log);
    void getIPbyIp();

private slots:
    void onDeviceConnected(bool success, const QString& serial, const QString& deviceName, const QSize& size);
    void onDeviceDisconnected(QString serial);

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
    void on_usbConnectBtn_clicked();
    void on_wifiConnectBtn_clicked();
    void on_connectedPhoneList_itemDoubleClicked(QListWidgetItem *item);
    void on_updateNameBtn_clicked();
    void on_useSingleModeCheck_clicked();
    void on_serialBox_currentIndexChanged(const QString &arg1);

    void on_startAudioBtn_clicked();

    void on_stopAudioBtn_clicked();

    void on_installSndcpyBtn_clicked();

    void on_autoUpdatecheckBox_toggled(bool checked);

    void on_codecModeBox_currentIndexChanged(int index);
    void on_mtkConfigBtn_clicked();

    void on_videoSourceBox_currentIndexChanged(int index);
    void on_refreshCameraBtn_clicked();
    void on_refreshAppsBtn_clicked();

    void showIpEditMenu(const QPoint &pos);

private:
    bool checkAdbRun();
    void initUI();
    void updateBootConfig(bool toView = true);
    void execAdbCmd();
    void delayMs(int ms);
    QString getGameScript(const QString &fileName);
    void slotActivated(QSystemTrayIcon::ActivationReason reason);
    int findDeviceFromeSerialBox(bool wifi);
    quint32 getBitRate();
    const QString &getServerPath();
    void updateVideoSourceUi();
    void initAdvancedDisplayUi();
    void updateAdvancedDisplayUi();
    void loadIpHistory();
    void saveIpHistory(const QString &ip);
    void loadPortHistory();
    void savePortHistory(const QString &port);

    void showPortEditMenu(const QPoint &pos);
    void syncMtkLevelToUi();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::Widget *ui;
    qsc::AdbProcess m_adb;
    QSystemTrayIcon *m_hideIcon;
    QGroupBox *m_advancedDisplayGroup = nullptr;
    QComboBox *m_displayModeBox = nullptr;
    QLineEdit *m_displayIdEdit = nullptr;
    QLineEdit *m_newDisplayEdit = nullptr;
    QLineEdit *m_cropEdit = nullptr;
    QCheckBox *m_flexDisplayCheck = nullptr;
    QComboBox *m_displayImePolicyBox = nullptr;
    QCheckBox *m_vdSystemDecorationsCheck = nullptr;
    QCheckBox *m_vdDestroyContentCheck = nullptr;
    QCheckBox *m_keepActiveCheck = nullptr;
    QComboBox *m_startAppBox = nullptr;
    QPushButton *m_refreshAppsBtn = nullptr;
    QMenu *m_menu;
    QAction *m_showWindow;
    QAction *m_quit;
    AudioOutput m_audioOutput;
    QTimer m_autoUpdatetimer;
    quint32 m_prevBitRate = 2000000;
    int m_prevMaxSizeIndex = 0;
    QPointer<MtkConfigDialog> m_mtkDialog;
};

#endif // DIALOG_H
