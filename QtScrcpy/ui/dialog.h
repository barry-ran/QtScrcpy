#ifndef DIALOG_H
#define DIALOG_H

#include <QWidget>
#include <QPointer>
#include <QMessageBox>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QListWidget>
#include <QStringList>
#include <QTimer>


#include "adbprocess.h"
#include "../QtScrcpyCore/include/QtScrcpyCore.h"
#include "audio/audiooutput.h"
#include "presetconfigdialog.h"

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
class QSpinBox;
class Dialog : public QWidget
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    void outLog(const QString &log, bool newLine = true);
    bool filterLog(const QString &log);
    void getIPbyIp();
    // show the main window (e.g. when hidden in the tray) and give it focus
    void bringToFront();

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
    void on_discoverBtn_clicked();
    void on_pairBtn_clicked();
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
    void on_presetConfigBtn_clicked();

    void on_videoSourceBox_currentIndexChanged(int index);
    void on_refreshCameraBtn_clicked();

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
    void initAdvancedConfigUi(QWidget *parent);
    void loadAdvancedConfig();
    void saveAdvancedConfig();
    void openUserDirectory(const QString &path);
    void refreshApps();
    void updateVideoSourceUi();
    void initAdvancedDisplayUi();
    void updateAdvancedDisplayUi();
    void loadPortHistory();
    void loadAddrHistory();
    void saveAddrHistory(const QString &addr);
    // 探测目标端口TCP是否可达，用于区分"端口不通"与"配对失效"
    bool probeTcp(const QString &host, quint16 port, int timeoutMs = 800);
    // 解析adb输出，给出可读结论
    void handleMdnsOutput(const QString &stdOut);
    void handlePairOutput(const QString &stdOut, const QString &errorOut);
    void handleConnectOutput(const QString &stdOut, const QString &errorOut, const QString &addr);
    // 发起无线连接；若上一条adb命令尚未收尾则稍后重试，避免链式调用被 checkAdbRun 拦截
    void requestWirelessConnect(int retry = 3);
    // adb空闲后再执行命令，避免被单实例拦截
    void execAdbWhenIdle(const QStringList &args, int retry = 20);
    // adb 的 mDNS 客户端在网络变化后会失效，kill-server + start-server 后重新查询
    void recoverAdbMdns(int step);
    // 端口已过期（手机重开无线调试/重新配对后端口会变）时，清掉端口框并重新向 mDNS 解析
    void retryWithFreshPort(const QString &host);
    // 当前是否有设备连接（有则不允许重启 adb server）
    bool hasConnectedDevice() const;

    void syncPresetLevelToUi();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::Widget *ui;
    qsc::AdbProcess m_adb;
    QSystemTrayIcon *m_hideIcon;
    QGroupBox *m_advancedDisplayGroup = nullptr;
    QComboBox *m_configLanguageBox = nullptr;
    QLineEdit *m_configTitleEdit = nullptr;
    QLineEdit *m_configPushPathEdit = nullptr;
    QSpinBox *m_configMaxFpsSpin = nullptr;
    QCheckBox *m_configRenderExpiredCheck = nullptr;
    QComboBox *m_configOpenGlBox = nullptr;
    QLineEdit *m_configServerPathEdit = nullptr;
    QLineEdit *m_configAdbPathEdit = nullptr;
    QLineEdit *m_configCodecOptionsEdit = nullptr;
    QLineEdit *m_configCodecNameEdit = nullptr;
    QComboBox *m_configLogLevelBox = nullptr;
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
    // 最近一次无线连接使用的地址，用于连接失败后做端口探测与原因归类
    QString m_lastConnectAddr;
    // 非空表示本次 mDNS 查询是为了补全该主机的端口，而不是做设备发现
    QString m_pendingConnectHost;
    // 本轮发现流程是否已经重启过 adb server，避免反复重启
    bool m_mdnsRecovered = false;
    // 本次连接是否已经因端口过期自动重解析过一次，避免无限重试
    bool m_portRetried = false;
    QPointer<PresetConfigDialog> m_presetDialog;
};

#endif // DIALOG_H
