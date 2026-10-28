#include <QtGlobal>
#ifdef Q_OS_MACOS
#include <CoreVideo/CoreVideo.h>
#endif
#include <cstring>
#include <memory>
#include <QWindow>
#include <QRegion>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaEnum>
#include <QtEndian>
#include <QEventLoop>
#include "QtScrcpyCore/src/device/controller/controller.h"
#include <QFile>
#include <QApplication>
#include <QInputMethodEvent>
#include <QLabel>
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#ifdef Q_OS_LINUX
#include <QIcon>
#endif
#include <QSurfaceFormat>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTranslator>
#include <QDateTime>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QLayout>
#include <QPushButton>
#include <QMenu>

#include "config.h"
#include "dialog.h"
#include "videoform.h"
#include "mousetap/mousetap.h"
#include "singleinstance.h"

static Dialog *g_mainDlg = Q_NULLPTR;
static QtMessageHandler g_oldMessageHandler = Q_NULLPTR;
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
void installTranslator();

static QtMsgType g_msgType = QtInfoMsg;
QtMsgType covertLogLevel(const QString &logLevel);

class ToolStabilityProbe : public QObject
{
public:
    using QObject::QObject;
    int moves=0,shows=0,hides=0;
    bool eventFilter(QObject *,QEvent *e) override {
        if(e->type()==QEvent::Move)++moves;
        if(e->type()==QEvent::Show)++shows;
        if(e->type()==QEvent::Hide)++hides;
        return false;
    }
};

class DesktopApplication : public QApplication
{
public:
    DesktopApplication(int &argc, char **argv) : QApplication(argc, argv) {}
    bool event(QEvent *event) override {
        if(event->type()==QEvent::Quit&&g_mainDlg)g_mainDlg->shutdownSessions();
        // ApplicationActivate also fires when clicking a projection or its tool
        // window. That is not a request to open the device/settings workspace.
        if (event->type() == QEvent::ApplicationActivate && g_mainDlg
            && (!g_mainDlg->isVisible() || g_mainDlg->isMinimized())) {
            QTimer::singleShot(0, g_mainDlg, []() {
                for (auto *window : QApplication::topLevelWidgets()) {
                    if (qobject_cast<VideoForm *>(window) && window->isVisible()) return;
                }
                if(g_mainDlg&&!g_mainDlg->isShuttingDown())g_mainDlg->bringToFront();
            });
        }
        return QApplication::event(event);
    }
};

int main(int argc, char *argv[])
{
    const bool exitSmoke = argc == 2 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--smoke-test-exit");
    const bool metalPreview=argc==2&&(QString::fromLocal8Bit(argv[1])==QStringLiteral("--preview-projection-metal")||QString::fromLocal8Bit(argv[1])==QStringLiteral("--smoke-test-projection-metal"));
    const bool projectionSmoke=argc==2&&(QString::fromLocal8Bit(argv[1])==QStringLiteral("--smoke-test-projection-ui")||QString::fromLocal8Bit(argv[1])==QStringLiteral("--smoke-test-projection-metal"));
    const bool imeSmoke=argc==2&&QString::fromLocal8Bit(argv[1])==QStringLiteral("--smoke-test-ime");
    const bool lifecycleSmoke=argc==2&&QString::fromLocal8Bit(argv[1])==QStringLiteral("--smoke-test-lifecycle");
    const bool projectionPreview = metalPreview || lifecycleSmoke || imeSmoke || projectionSmoke || (argc == 2 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--preview-projection-ui"));
    const bool keymapSmoke=argc==2&&QString::fromLocal8Bit(argv[1])==QStringLiteral("--smoke-test-keymap");
    const bool isolatedMode = exitSmoke || projectionPreview || keymapSmoke;
    QCoreApplication::setOrganizationName("QtScrcpy");
    QCoreApplication::setApplicationName(isolatedMode ? "QtScrcpyLocalExitTest" : "QtScrcpyLocal");

    const QString executableDir = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath();
#ifdef Q_OS_MACOS
    const QString repositoryRoot = QDir(executableDir).absoluteFilePath("../../../../../../");
#else
    const QString repositoryRoot = QDir(executableDir).absoluteFilePath("../../..");
#endif
    const QDir repositoryDir(repositoryRoot);
    const QString defaultKeymapPath = repositoryDir.exists("keymap")
        ? repositoryDir.filePath("keymap")
        : QDir(executableDir).filePath("keymap");
    const QString defaultConfigPath = QFileInfo(repositoryDir.filePath("config/config.ini")).isFile()
        ? repositoryDir.filePath("config")
        : QDir(executableDir).filePath("config");

    // Keep packaged defaults separate from mutable user data. Development
    // builds use the repository templates, while release packages use files
    // beside the executable. AppImage AppRun may override these paths.
    if (qgetenv("QTSCRCPY_DEFAULT_KEYMAP_PATH").isEmpty()) {
        qputenv("QTSCRCPY_DEFAULT_KEYMAP_PATH", defaultKeymapPath.toLocal8Bit());
    }
    if (qgetenv("QTSCRCPY_DEFAULT_CONFIG_PATH").isEmpty()) {
        qputenv("QTSCRCPY_DEFAULT_CONFIG_PATH", defaultConfigPath.toLocal8Bit());
    }

    // set env
#ifdef Q_OS_WIN32
    qputenv("QTSCRCPY_ADB_PATH", "../../../QtScrcpy/QtScrcpyCore/src/third_party/adb/win/adb.exe");
    qputenv("QTSCRCPY_SERVER_PATH", "../../../QtScrcpy/QtScrcpyCore/src/third_party/scrcpy-server");
#endif

#ifdef Q_OS_MACOS
    qputenv("QTSCRCPY_ADB_PATH", "../../../../../../QtScrcpy/QtScrcpyCore/src/third_party/adb/mac/adb");
    qputenv("QTSCRCPY_SERVER_PATH", "../../../../../../QtScrcpy/QtScrcpyCore/src/third_party/scrcpy-server");
#endif

#ifdef Q_OS_LINUX
    // Only set environment variables if they are not already set (e.g., by AppImage AppRun)
    if (qgetenv("QTSCRCPY_ADB_PATH").isEmpty()) {
        qputenv("QTSCRCPY_ADB_PATH", "../../../QtScrcpy/QtScrcpyCore/src/third_party/adb/linux/adb");
    }
    if (qgetenv("QTSCRCPY_SERVER_PATH").isEmpty()) {
        qputenv("QTSCRCPY_SERVER_PATH", "../../../QtScrcpy/QtScrcpyCore/src/third_party/scrcpy-server");
    }
#endif

    g_msgType = covertLogLevel(Config::getInstance().getLogLevel());

    // set on QApplication before
    // bug: config path is error on mac
    int opengl = Config::getInstance().getDesktopOpenGL();
    if (0 == opengl) {
        QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    } else if (1 == opengl) {
        QApplication::setAttribute(Qt::AA_UseOpenGLES);
    } else if (2 == opengl) {
        QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#endif

    QSurfaceFormat varFormat = QSurfaceFormat::defaultFormat();
    varFormat.setVersion(2, 0);
    varFormat.setProfile(QSurfaceFormat::NoProfile);
    /*
    varFormat.setSamples(4);
    varFormat.setAlphaBufferSize(8);
    varFormat.setBlueBufferSize(8);
    varFormat.setRedBufferSize(8);
    varFormat.setGreenBufferSize(8);
    varFormat.setDepthBufferSize(24);
    */
    QSurfaceFormat::setDefaultFormat(varFormat);

    g_oldMessageHandler = qInstallMessageHandler(myMessageOutput);
    DesktopApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    // Only one QtScrcpy per user. If one is already running (possibly hidden in
    // the system tray), ask it to show its window and exit.
    SingleInstance singleInstance(isolatedMode ? QStringLiteral("QtScrcpyLocalExitTest") : QStringLiteral("QtScrcpyLocal"));
    if (!singleInstance.isPrimary()) {
        singleInstance.notifyPrimary();
        return 0;
    }

    // Keep Qt window and macOS Dock icons consistent with the bundle icon.
    // Load icon from Qt resource (logo.png is included in res.qrc)
    QIcon appIcon(":/image/tray/logo.png");
    if (!appIcon.isNull()) {
        a.setWindowIcon(appIcon);
    }

    // windows下通过qmake VERSION变量或者rc设置版本号和应用名称后，这里可以直接拿到
    // mac下拿到的是CFBundleVersion的值
    qDebug() << a.applicationVersion();
    qDebug() << a.applicationName();

    //update version
    QStringList versionList = QCoreApplication::applicationVersion().split(".");
    if (versionList.size() >= 3) {
        QString version = versionList[0] + "." + versionList[1] + "." + versionList[2];
        a.setApplicationVersion(version);
    }

    installTranslator();
#if defined(Q_OS_WIN32) || defined(Q_OS_MACOS)
    if (!isolatedMode) MouseTap::getInstance()->initMouseEventTap();
#endif

    qApp->setStyle(QStringLiteral("Fusion"));
    // Shared desktop design system.
    QPalette modernPalette;
    modernPalette.setColor(QPalette::Window, QColor("#fcfcfa"));
    modernPalette.setColor(QPalette::WindowText, QColor("#303832"));
    modernPalette.setColor(QPalette::Base, QColor("#ffffff"));
    modernPalette.setColor(QPalette::Text, QColor("#303832"));
    modernPalette.setColor(QPalette::Button, QColor("#ffffff"));
    modernPalette.setColor(QPalette::ButtonText, QColor("#303832"));
    modernPalette.setColor(QPalette::Highlight, QColor("#6b886f"));
    modernPalette.setColor(QPalette::HighlightedText, Qt::white);
    qApp->setPalette(modernPalette);
    qApp->setStyleSheet(QStringLiteral(R"(
        QWidget { font-family: ".AppleSystemUIFont", "PingFang SC"; font-size: 13px; font-weight: 400; color: #303832; }
        QWidget#Workspace, QWidget#Page { background: #fcfcfa; }
        QFrame#Sidebar { background: #efeee9; border: none; border-right: 1px solid #e2e2dc; }
        QFrame#Footer { background: #f7f7f3; border-top: 1px solid #e2e2dc; }
        QFrame#Divider { background: #e5e6df; border: none; }
        QLabel { background: transparent; border: none; }
        QLabel[role="deviceTitle"] { font-size: 19px; font-weight: 600; color: #303a33; }
        QLabel[role="section"] { font-size: 15px; font-weight: 600; color: #414c42; }
        QLabel[role="muted"] { color: #6a736b; font-size: 12px; }
        QLabel[role="connection"] { color: #55745c; font-size: 12px; }
        QPushButton { background: #f9faf6; border: 1px solid #d4d9d0; border-radius: 7px; padding: 6px 13px; color: #465347; }
        QPushButton:hover { background: #edf1e8; border-color: #9dab98; }
        QPushButton:pressed { background: #dfe7d9; }
        QPushButton:focus { border-color: #7b9677; }
        QPushButton:disabled { color: #969e92; background: #f1f2ec; border-color: #e0e3d9; }
        QPushButton#PrimaryButton { background: #6b886f; border-color: #6b886f; color: #ffffff; font-weight: 600; }
        QPushButton#PrimaryButton:hover { background: #58775e; }
        QPushButton#PrimaryButton:disabled { background: #afbbaa; border-color: #afbbaa; color: #ffffff; }
        QPushButton#LinenTab { background: transparent; border: none; border-bottom: 2px solid transparent; border-radius: 0; padding: 7px 3px 12px; color: #727c71; }
        QPushButton#LinenTab:hover { color: #3f5e45; }
        QPushButton#LinenTab:checked { border-bottom-color: #6b886f; color: #496d50; font-weight: 600; }
        QPushButton#TextButton { background: transparent; border: none; padding: 0 4px; color: #5e7460; }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { background: #fafbf7; border: 1px solid #d8ddd3; border-radius: 6px; padding: 6px 10px; selection-background-color: #6b886f; }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border-color: #7b9677; }
        QComboBox::drop-down { border: none; width: 24px; }
        QComboBox::down-arrow { image: url(:/qss/modern-arrow.svg); width: 11px; height: 11px; }
        QComboBox QAbstractItemView { background: #fcfcfa; color: #303832; selection-background-color: #e5eddf; selection-color: #3c5b42; border: 1px solid #d8ddd3; }
        QCheckBox { background: transparent; spacing: 9px; padding: 4px 0; }
        QCheckBox::indicator { width: 17px; height: 17px; background: #fafbf7; border: 1px solid #b9c4b3; border-radius: 5px; }
        QCheckBox::indicator:checked { background: #6b886f; border-color: #6b886f; image: url(:/qss/modern-check.svg); }
        QComboBox:disabled, QLineEdit:disabled { background: #f1f2ed; color: #939d8d; border-color: #e0e4d8; }
        QGroupBox { background: #fcfcfa; border: 1px solid #e1e5db; border-radius: 8px; margin-top: 15px; padding: 17px; }
        QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 5px; color: #526450; }
        QPlainTextEdit, QTextEdit { background: #f3f5ee; border: 1px solid #dce2d4; border-radius: 6px; padding: 9px; color: #465c43; font-size: 11px; }
        QTextEdit#DiagnosticLog { background: #f5f5f0; color: #38473b; font-size: 12px; padding: 12px; selection-background-color: #dfe7d9; selection-color: #303832; }
        QMenu { background: #fcfcfa; color: #303832; border: 1px solid #d8ddd3; padding: 5px; }
        QMenu::item { padding: 7px 18px; border-radius: 4px; }
        QMenu::item:selected { background: #e5eddf; color: #3c5b42; }
        QMenu::item:disabled { color: #939d8d; }
        QMenu::separator { height: 1px; background: #e2e2dc; margin: 4px 8px; }
        QListWidget { background: transparent; border: none; outline: none; }
        QListWidget::item { padding: 7px; border-radius: 6px; margin: 1px 0; font-size: 11px; }
        QListWidget::item:selected { background: #dfe7d9; color: #405d43; }
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical { background: transparent; width: 7px; margin: 2px; }
        QScrollBar::handle:vertical { background: #c7cebf; border-radius: 3px; min-height: 28px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QTabWidget::pane { border: none; background: #fcfcfa; }
        QTabBar::tab { padding: 9px 12px; color: #6c7968; border: none; border-bottom: 2px solid transparent; }
        QTabBar::tab:selected { color: #496d50; border-bottom-color: #6b886f; }
        QWidget#ToolForm { background: #f3f3ed; border: 1px solid #d7ddd0; border-radius: 9px; }
        QWidget#ToolForm QPushButton { padding: 0; border: none; }
        QWidget#ToolForm QPushButton:hover { background: #e0e8d9; border-radius: 6px; }
        QToolTip { color: #ffffff; background: #4c5f49; border: none; padding: 6px; }
    )"));

    qsc::AdbProcess::setAdbPath(Config::getInstance().getAdbPath());

    g_mainDlg = new Dialog {};
    g_mainDlg->setWindowTitle(QStringLiteral("QtScrcpy · 本地增强版"));
    g_mainDlg->show();
    QObject::connect(&singleInstance, &SingleInstance::activateRequested, g_mainDlg, &Dialog::bringToFront);

    qInfo() << QObject::tr("This software is completely open source and free. Use it at your own risk. You can download it at the "
            "following address:");
    qInfo() << QString("QtScrcpy %1 <https://github.com/barry-ran/QtScrcpy>").arg(QCoreApplication::applicationVersion());

    qInfo() << QStringLiteral("本地增强版：音频联动 / 断连恢复 / 关闭窗口后台运行");

    if (keymapSmoke) QTimer::singleShot(100, &a, [&]() {
        QFile file(QCoreApplication::applicationDirPath()+"/keymap/gameforpeace-k80-f8.json");
        if(!file.open(QIODevice::ReadOnly))qFatal("K80 keymap missing");
        QList<QByteArray> packets;
        const QByteArray script=file.readAll();
        const auto switchName=QJsonDocument::fromJson(script).object()["switchKey"].toString();
        const int switchCode=QMetaEnum::fromType<Qt::Key>().keyToValue(switchName.toLatin1().constData());
        if(switchCode<0)qFatal("Invalid configured switch key");
        Controller controller([&](const QByteArray &packet){packets.append(packet);return qint64(packet.size());},QString::fromUtf8(script));
        auto key=[&](int code,QEvent::Type type,bool repeat=false){
            QKeyEvent event(type,code,Qt::NoModifier,QString(),repeat);
            controller.keyEvent(&event,QSize(2844,1280),QSize(1422,640));
            QCoreApplication::sendPostedEvents(&controller);
        };
        key(switchCode,QEvent::KeyPress);
        if(!controller.isCurrentCustomKeymap())qFatal("Switch key failed to enable map");
        key(switchCode,QEvent::KeyPress,true);
        if(!controller.isCurrentCustomKeymap())qFatal("Switch key auto repeat toggled map");
        packets.clear();key(Qt::Key_Q,QEvent::KeyPress);key(Qt::Key_E,QEvent::KeyPress);
        if(packets.size()!=2||packets[0].at(0)!=2||packets[1].at(0)!=2||packets[0].mid(2,8)==packets[1].mid(2,8))qFatal("Lean keys did not allocate independent touch IDs");
        key(Qt::Key_Q,QEvent::KeyRelease);key(Qt::Key_E,QEvent::KeyRelease);
        for(int code:{Qt::Key_Tab,Qt::Key_M}) {
            packets.clear();key(code,QEvent::KeyPress);key(code,QEvent::KeyRelease);
            if(packets.isEmpty()||packets.first().at(0)!=2)qFatal("Panel open did not inject touch");
            packets.clear();key(code,QEvent::KeyPress);key(code,QEvent::KeyRelease);
            if(packets.size()!=2||packets[0].at(0)!=0||packets[0].mid(2,4)!=QByteArray::fromHex("00000004"))qFatal("Panel close did not send Android Back");
        }
        auto mouse=[&](Qt::MouseButton button,QEvent::Type type){
            QMouseEvent event(type,QPointF(800,300),QPointF(800,300),button,type==QEvent::MouseButtonRelease?Qt::NoButton:button,Qt::NoModifier);
            controller.mouseEvent(&event,QSize(2844,1280),QSize(1422,640));
            QCoreApplication::sendPostedEvents(&controller);
        };
        auto number=[](const QByteArray &p,int offset){return qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(p.constData()+offset));};
        int tested=0;
        for(const auto value:QJsonDocument::fromJson(script).object()["keyMapNodes"].toArray()) {
            const auto node=value.toObject();if(node["type"].toString()!="KMT_CLICK")continue;
            packets.clear();const auto name=node["key"].toString();
            if(name.endsWith("Button")) {
                const auto button=name=="LeftButton"?Qt::LeftButton:Qt::RightButton;
                mouse(button,QEvent::MouseButtonPress);mouse(button,QEvent::MouseButtonRelease);
            } else {
                const int code=QMetaEnum::fromType<Qt::Key>().keyToValue(name.toLatin1().constData());
                if(code<0)qFatal("Unknown configured key");
                key(code,QEvent::KeyPress);key(code,QEvent::KeyRelease);
            }
            const auto pos=node["pos"].toObject();bool down=false,up=false;
            for(const auto &packet:packets)if(packet.size()>=18&&packet[0]==2) {
                if(packet[1]==0) {
                    down=true;
                    if(qAbs(int(number(packet,10))-qRound(pos["x"].toDouble()*2844))>1||qAbs(int(number(packet,14))-qRound(pos["y"].toDouble()*1280))>1)qFatal("Mapped coordinate differs from configured coordinate");
                }
                if(packet[1]==1)up=true;
            }
            if(!down||!up)qFatal("Configured click missing down/up: %s",qPrintable(name));
            ++tested;
            key(switchCode,QEvent::KeyPress);key(switchCode,QEvent::KeyPress);
        }
        qInfo()<<"Every configured click passed:"<<tested;
        packets.clear();key(Qt::Key_W,QEvent::KeyPress);key(Qt::Key_D,QEvent::KeyPress);
        key(Qt::Key_Q,QEvent::KeyPress);mouse(Qt::LeftButton,QEvent::MouseButtonPress);mouse(Qt::RightButton,QEvent::MouseButtonPress);key(Qt::Key_Space,QEvent::KeyPress);
        QEventLoop tick;QTimer::singleShot(250,&tick,&QEventLoop::quit);tick.exec();QCoreApplication::sendPostedEvents(&controller);
        QSet<QByteArray> active;int peak=0,moves=0;
        auto track=[&](){for(const auto &packet:packets) { if(packet.size()>=28&&packet[0]==2){
            const auto id=packet.mid(2,8);const int action=uchar(packet[1]);
            if(action==0){if(active.contains(id))qFatal("Duplicate live touch ID");active.insert(id);peak=qMax(peak,int(active.size()));}
            else {if(!active.contains(id))qFatal("Move/up without active touch");if(action==1)active.remove(id);if(action==2){++moves;if(packet.mid(22,2)==QByteArray(2,0))qFatal("Held touch move had zero pressure");}}
        }}
        packets.clear();};
        track();if(peak<5||moves==0)qFatal("Combined movement/lean/fire/aim/jump failed");
        key(switchCode,QEvent::KeyPress);track();if(!active.isEmpty())qFatal("Mode switch left held touches down");
        key(switchCode,QEvent::KeyPress);packets.clear();
        QWheelEvent wheel(QPointF(500,300),QPointF(500,300),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        controller.wheelEvent(&wheel,QSize(2844,1280),QSize(1422,640));QCoreApplication::sendPostedEvents(&controller);
        if(packets.size()!=2||packets.first()[0]!=2)qFatal("Wheel weapon selection failed");
        packets.clear();key(Qt::Key_Control,QEvent::KeyPress);key(Qt::Key_Control,QEvent::KeyRelease);
        if(!controller.isCurrentCustomKeymap())qFatal("Cursor switch disabled map");
        mouse(Qt::LeftButton,QEvent::MouseButtonPress);mouse(Qt::LeftButton,QEvent::MouseButtonRelease);
        if(packets.isEmpty()||number(packets.first(),10)!=1600)qFatal("Released cursor did not click actual pointer position");
        key(Qt::Key_Control,QEvent::KeyPress);key(Qt::Key_Control,QEvent::KeyRelease);
        for(int direction:{Qt::Key_W,Qt::Key_A,Qt::Key_S,Qt::Key_D}) {
            packets.clear();key(direction,QEvent::KeyPress);
            QEventLoop settle;QTimer::singleShot(250,&settle,&QEventLoop::quit);settle.exec();QCoreApplication::sendPostedEvents(&controller);
            key(direction,QEvent::KeyRelease);active.clear();peak=0;moves=0;track();
            if(peak!=1||moves==0||!active.isEmpty())qFatal("Individual WASD direction failed");
        }
        packets.clear();active.clear();peak=0;moves=0;
        for(int held:{Qt::Key_Q,Qt::Key_E,Qt::Key_C,Qt::Key_Z,Qt::Key_R,Qt::Key_1,Qt::Key_2,Qt::Key_3,Qt::Key_4,Qt::Key_5,Qt::Key_6})key(held,QEvent::KeyPress);
        track();if(peak!=10)qFatal("10-touch capacity or overflow handling failed");
        key(switchCode,QEvent::KeyPress);track();if(!active.isEmpty())qFatal("10-touch release failed");
        key(switchCode,QEvent::KeyPress);
        packets.clear();key(Qt::Key_Alt,QEvent::KeyPress);
        QEventLoop eye;QTimer::singleShot(100,&eye,&QEventLoop::quit);eye.exec();QCoreApplication::sendPostedEvents(&controller);
        bool eyeDown=false;for(const auto &packet:packets)if(packet.size()>18&&packet[0]==2&&packet[1]==0)eyeDown=true;
        if(!eyeDown)qFatal("Alt free-look failed to start touch");
        key(Qt::Key_Alt,QEvent::KeyRelease);
        key(switchCode,QEvent::KeyPress);key(switchCode,QEvent::KeyPress);
        qInfo()<<"Alt free-look touch start/release exercised";
        auto settleSprint=[&](){QEventLoop loop;QTimer::singleShot(500,&loop,&QEventLoop::quit);loop.exec();QCoreApplication::sendPostedEvents(&controller);};
        auto lastTouchY=[&](){int y=-1;for(const auto &p:packets)if(p.size()>18&&p[0]==2&&p[1]==2)y=int(number(p,14));return y;};
        for(bool shiftFirst:{false,true}) {
            packets.clear();active.clear();peak=0;moves=0;
            if(shiftFirst){key(Qt::Key_Shift,QEvent::KeyPress);if(!packets.isEmpty())qFatal("Shift alone generated touch");}
            key(Qt::Key_W,QEvent::KeyPress);settleSprint();
            if(!shiftFirst){key(Qt::Key_Shift,QEvent::KeyPress);settleSprint();}
            const int sprintY=lastTouchY();if(sprintY<0||sprintY>500)qFatal("Shift W did not extend joystick");
            key(Qt::Key_Shift,QEvent::KeyRelease);settleSprint();
            const int walkY=lastTouchY();if(walkY<sprintY+250)qFatal("Shift release did not restore normal movement");
            key(Qt::Key_W,QEvent::KeyRelease);track();if(peak!=1||!active.isEmpty())qFatal("Sprint must reuse and release one touch");
        }
        qInfo()<<"Shift/W both press orders, release-to-walk and single-touch continuity passed";
        qInfo()<<"All WASD directions and 10-touch capacity/overflow/release passed";
        qInfo()<<"Combined 5-touch stream, nonzero move pressure, cleanup, wheel and free cursor passed";
        key(switchCode,QEvent::KeyPress);
        qInfo()<<"K80 mapping: repeat guard, independent lean touches and panel close protocol passed";
        // Unwind local controllers/nested test loops before stopping Cocoa.
        QTimer::singleShot(0,&a,&QCoreApplication::quit);
    });
    if (exitSmoke) {
        // Exercise the real close/destruction path with isolated settings and no input hooks.
        QTimer::singleShot(200, g_mainDlg, &QWidget::close);
        QTimer::singleShot(400, &a, [&a]() {
            if (!g_mainDlg || g_mainDlg->isVisible()) qFatal("Close must hide the main window");
            qInfo() << "Close preserved application; native Quit event follows";
            QCoreApplication::postEvent(&a,new QEvent(QEvent::Quit));
        });
    }
    QPointer<VideoForm> preview;
    if (projectionPreview) {
        g_mainDlg->hide();
        preview = new VideoForm(true, false, true, metalPreview?1:0);
        preview->setWindowTitle(QStringLiteral("界面预览 · 未连接设备"));
        preview->updateShowSize(QSize(288,640));
        preview->show();
#ifdef Q_OS_MACOS
        if(metalPreview) {
            auto *frames=new QTimer(preview);frames->setObjectName("MetalFixtureFrames");frames->setInterval(33);
            QObject::connect(frames,&QTimer::timeout,preview,[preview](){
                if(!preview||preview->isMinimized())return;
                const QSize size=preview->frameSize();
                CFMutableDictionaryRef attributes=CFDictionaryCreateMutable(kCFAllocatorDefault,1,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
                CFDictionaryRef surface=CFDictionaryCreate(kCFAllocatorDefault,nullptr,nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
                CFDictionarySetValue(attributes,kCVPixelBufferIOSurfacePropertiesKey,surface);
                CVPixelBufferRef buffer=nullptr;
                const auto result=CVPixelBufferCreate(kCFAllocatorDefault,size.width(),size.height(),kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,attributes,&buffer);
                CFRelease(surface);CFRelease(attributes);
                if(result!=kCVReturnSuccess)qFatal("Could not create isolated Metal fixture");
                CVPixelBufferLockBaseAddress(buffer,0);
                std::memset(CVPixelBufferGetBaseAddressOfPlane(buffer,0),205,CVPixelBufferGetBytesPerRowOfPlane(buffer,0)*size.height());
                std::memset(CVPixelBufferGetBaseAddressOfPlane(buffer,1),128,CVPixelBufferGetBytesPerRowOfPlane(buffer,1)*size.height()/2);
                CVPixelBufferUnlockBaseAddress(buffer,0);
                static_cast<qsc::DeviceObserver*>(preview.data())->onFrameMetal(buffer,size.width(),size.height());
                CVPixelBufferRelease(buffer);
            });frames->start();
        }
#endif
        QTimer::singleShot(100, preview, [preview]() {
            if (!preview) return;
            static QByteArray y(288*640, char(205));
            static QByteArray u(144*320, char(125));
            static QByteArray v(144*320, char(127));
            preview->updateRender(288,640,reinterpret_cast<uint8_t*>(y.data()),reinterpret_cast<uint8_t*>(u.data()),reinterpret_cast<uint8_t*>(v.data()),288,144,144);
        });
    }
    if(lifecycleSmoke)QTimer::singleShot(750,&a,[&](){
        auto closes=std::make_shared<int>(0);
        preview->setSerial(QStringLiteral("lifecycle-synthetic"));
        QObject::connect(preview,&VideoForm::sessionCloseRequested,&a,[closes](const QString &serial){
            if(serial!=QStringLiteral("lifecycle-synthetic"))qFatal("Wrong session close target");
            ++*closes;
        });
        const QPointer<Dialog> workspace=g_mainDlg;
        g_mainDlg->show();g_mainDlg->close();
        if(!workspace||workspace->isVisible()||workspace->isShuttingDown()||!preview->isVisible()||*closes)qFatal("Workspace close ended a session/application");
        QTimer::singleShot(150,&a,[&,workspace,closes](){
            if(!workspace||workspace->isVisible()||workspace->isShuttingDown()||!preview->isVisible())qFatal("Background application did not survive workspace close");
            preview->showProjectionControls();preview->minimizeProjection();
            if(*closes)qFatal("Minimize requested session end");
            QTimer::singleShot(350,&a,[&,workspace,closes](){
                if(*closes)qFatal("Session ended while minimized");
                preview->showNormal();preview->close();preview->close();
                if(*closes!=1||workspace->isVisible()||workspace->isShuttingDown())qFatal("Session close must be idempotent and preserve background application");
                VideoForm disconnected(true,false,true,0);
                int redundantClose=0;
                QObject::connect(&disconnected,&VideoForm::sessionCloseRequested,[&](const QString &){++redundantClose;});
                disconnected.finishSession();
                if(redundantClose)qFatal("Disconnect cleanup requested a second disconnect");
                QPointer<QProcess> pendingA=new QProcess(workspace),pendingB=new QProcess(workspace);
                pendingA->setProperty("sessionSerial",QStringLiteral("pending-device-A"));pendingB->setProperty("sessionSerial",QStringLiteral("pending-device-B"));
                pendingA->start("/bin/sleep",{QStringLiteral("5")});pendingB->start("/bin/sleep",{QStringLiteral("5")});
                if(!pendingA->waitForStarted(1000)||!pendingB->waitForStarted(1000))qFatal("Could not start isolated lifecycle child tasks");
                workspace->endProjection(QStringLiteral("pending-device-A"));
                if(pendingA&&pendingA->state()!=QProcess::NotRunning)qFatal("Ended session left pending startup alive");
                if(!pendingB||pendingB->state()==QProcess::NotRunning)qFatal("Ending A stopped B's pending work");
                workspace->endProjection(QStringLiteral("missing-test-device"));
                if(workspace->isVisible()||workspace->isShuttingDown())qFatal("Device loss changed workspace/background lifetime");
                qInfo()<<"Lifecycle: workspace close preserved application/projection; minimize preserved session; session close emitted once; disconnect cleanup did not reopen workspace";
                workspace->requestQuit();
                if(!workspace->isShuttingDown())qFatal("Explicit quit skipped shutdown pipeline");
                if(pendingB&&pendingB->state()!=QProcess::NotRunning)qFatal("Quit left own startup task alive");
            });
        });
    });
    if(imeSmoke)QTimer::singleShot(750,&a,[&](){
            QInputMethodEvent composing(QStringLiteral("nihao"), {});
            QApplication::sendEvent(preview,&composing);
            if(!composing.isAccepted())qFatal("IME preedit rejected");
            if(preview->findChild<QLabel*>(QStringLiteral("DesktopImePreedit")))qFatal("Duplicate IME overlay present");
            QInputMethodEvent committed;committed.setCommitString(QStringLiteral("你好"));
            QApplication::sendEvent(preview,&committed);
            if(!committed.isAccepted())qFatal("IME commit rejected");
            qInfo()<<"Desktop IME preedit and commit checks passed";a.quit();
    });
    if(projectionSmoke) {
        QTimer::singleShot(750,&a,[&](){
            if(!preview)qFatal("Projection missing");
            if(metalPreview){bool rendered=false;for(auto *child:preview->findChildren<QWidget*>())rendered|=child->property("renderedFrames").toInt()>0;if(!rendered)qFatal("Metal renderer did not draw in transparent single host");}
            preview->activateWindow();preview->showProjectionControls();
            auto *title=preview->findChild<QWidget*>("ProjectionTitle");
            auto *tools=preview->findChild<QWidget*>("ToolForm");
            auto *corner=preview->findChild<QWidget*>("ProjectionTopLeftGrip");
            if(!title||!tools||!corner)qFatal("Projection controls missing");
            if(title->isWindow()||tools->isWindow()||title->window()!=preview||tools->window()!=preview)qFatal("Chrome still uses independent native windows");
            const auto assertAspect=[&](){
                const QRect phone=preview->phoneRect();
                const QSize frame=preview->frameSize();
                if(!preview->rect().contains(phone)||qAbs(phone.width()-phone.height()*qreal(frame.width())/frame.height())>1.5)qFatal("Phone image aspect or containment failed");
            };
            assertAspect();
            const QRect phone=preview->phoneRect(), bounds=preview->geometry();
            if(title->geometry().bottom()+1!=phone.top())qFatal("Title gap/overlap remains");
            preview->hideProjectionControls();
            if(preview->phoneRect()!=phone||preview->geometry()!=bounds||preview->mask()!=QRegion(phone))qFatal("Hidden controls shifted phone or retained empty hit area");
            preview->showProjectionControls();
            if(preview->phoneRect()!=phone||!preview->mask().contains(title->geometry())||!preview->mask().contains(tools->geometry()))qFatal("Controls did not reveal in host mask");
            if(preview->mask().contains(QPoint(preview->width()-1,preview->height()-1)))qFatal("Unused transparent area still intercepts input");
            int primary=0;
            for(auto *button:tools->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly))if(!button->isHidden())++primary;
            if(primary!=7)qFatal("Primary phone tools changed unexpectedly");
            auto *audio=tools->findChild<QPushButton*>("ProjectionAudioToggle");
            preview->setAudioState(true,false);if(!audio||!audio->isChecked())qFatal("Audio tool state missing");preview->setAudioState(false,false);
            QPushButton *more=nullptr;
            for(auto *button:tools->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly))if(button->accessibleName()==QStringLiteral("更多工具"))more=button;
            if(!more)qFatal("More tool missing");
            more->click();
            auto *popup=qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if(!popup||!popup->isVisible()||popup->actions().size()!=15||popup->parentWidget()!=preview||tools->isWindow()||tools->window()!=preview)qFatal("More menu broke single host ownership");
            const QRect stripGlobal(tools->mapToGlobal(QPoint()),tools->size());
            if(qAbs(popup->pos().x()-stripGlobal.right()-1)>2&&qAbs(popup->geometry().right()+1-stripGlobal.left())>2)qFatal("More menu detached from tools");
            popup->close();
            auto *probe=new ToolStabilityProbe(tools);tools->installEventFilter(probe);
            const QRect localTools=tools->geometry();
            const QPoint globalBefore=tools->mapToGlobal(QPoint());
            const QPoint windowBefore=preview->pos();
            const QPointF titleLocal(130,16),titleGlobal=title->mapToGlobal(titleLocal.toPoint());
            QMouseEvent titleDown(QEvent::MouseButtonPress,titleLocal,titleGlobal,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(title,&titleDown);
            QMouseEvent titleMove(QEvent::MouseMove,titleLocal+QPointF(30,20),titleGlobal+QPointF(30,20),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(title,&titleMove);
            QMouseEvent titleUp(QEvent::MouseButtonRelease,titleLocal+QPointF(30,20),titleGlobal+QPointF(30,20),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(title,&titleUp);
            if(tools->geometry()!=localTools||tools->mapToGlobal(QPoint())-globalBefore!=preview->pos()-windowBefore||probe->hides||probe->shows)qFatal("Single-host drag detached or flickered tools");
            QWindow nativeSurface;
            for(int n=0;n<10;++n) {
                const QPointF local=preview->phoneRect().center(),global=preview->mapToGlobal(local.toPoint());
                QMouseEvent nativeDown(QEvent::MouseButtonPress,local,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&nativeSurface,&nativeDown);
                QMouseEvent down(QEvent::MouseButtonPress,local,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(preview,&down);
                QMouseEvent up(QEvent::MouseButtonRelease,local,global,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(preview,&up);
            }
            if(probe->hides||probe->shows)qFatal("Phone clicks flickered child controls");
            const QRect beforeResize=preview->geometry();
            const QPointF gripLocal(4,4),gripGlobal=corner->mapToGlobal(QPoint(4,4));
            QMouseEvent gripDown(QEvent::MouseButtonPress,gripLocal,gripGlobal,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(corner,&gripDown);
            QMouseEvent gripMove(QEvent::MouseMove,gripLocal+QPointF(-24,-44),gripGlobal+QPointF(-24,-44),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(corner,&gripMove);
            QMouseEvent gripUp(QEvent::MouseButtonRelease,gripLocal,gripGlobal,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(corner,&gripUp);
            if(preview->geometry().bottomRight()!=beforeResize.bottomRight()||preview->width()<=beforeResize.width())qFatal("Corner resize lost opposite anchor");
            assertAspect();
            preview->resize(preview->windowSizeForPhone(QSize(350,400)));
            QTimer::singleShot(150,&a,[&](){
                const QRect phone=preview->phoneRect();if(qAbs(phone.width()-phone.height()*(288.0/640))>1.5)qFatal("Manual resize lost phone aspect");
                preview->updateShowSize(QSize(1280,720));preview->showProjectionControls();
                QTimer::singleShot(150,&a,[&](){
                    auto *title=preview->findChild<QWidget*>("ProjectionTitle");auto *tools=preview->findChild<QWidget*>("ToolForm");
                    if(title->geometry().bottom()+1!=preview->phoneRect().top()||tools->geometry().bottom()+1!=title->geometry().top())qFatal("Landscape controls are not outside phone in one host");
                    const QRect normal=preview->geometry();preview->switchFullScreen();
                    QTimer::singleShot(1800,&a,[&,normal](){
                        auto *title=preview->findChild<QWidget*>("ProjectionTitle");
                        auto *host=preview->findChild<QWidget*>("keepRatioWidget");
                        const QRect phone=preview->phoneRect();
                        if(!preview->isFullScreen()||title->isVisible()||host->geometry()!=preview->rect()||!preview->mask().isEmpty()||qAbs(phone.width()-phone.height()*(1280.0/720))>1.5)qFatal("Fullscreen host has chrome margin/stretch");
                        if(qAbs(phone.center().x()-preview->rect().center().x())>1||qAbs(phone.center().y()-preview->rect().center().y())>1)qFatal("Fullscreen image not centered");
                        if(metalPreview){for(auto *child:preview->findChildren<QWidget*>())if(child->property("renderedFrames").isValid()&&!child->property("nativeGeometryMatchesQt").toBool())qFatal("Metal native view escaped Qt centering");}
                        preview->updateShowSize(QSize(720,1280));
                        const QRect portrait=preview->phoneRect();
                        if(!preview->isFullScreen()||qAbs(portrait.center().x()-preview->rect().center().x())>1||qAbs(portrait.center().y()-preview->rect().center().y())>1)qFatal("Fullscreen rotation lost centering/state");
                        preview->updateShowSize(QSize(1280,720));
                        auto *tools=preview->findChild<QWidget*>("ToolForm");
                        int count=0;for(auto *button:tools->findChildren<QPushButton*>(QString(),Qt::FindDirectChildrenOnly))if(!button->isHidden())++count;
                        if(count!=8||tools->styleSheet().isEmpty())qFatal("Fullscreen controls missing exit/style");
                        preview->hideProjectionControls();
                        QMouseEvent phoneClick(QEvent::MouseButtonPress,QPointF(phone.center()),QPointF(preview->mapToGlobal(phone.center())),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(preview,&phoneClick);
                        if(tools->isVisible())qFatal("Fullscreen phone click revealed toolbar");
                        QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(preview,&escape);
                        QTimer::singleShot(1800,&a,[&,normal](){
                            if(preview->isFullScreen()||preview->geometry()!=normal)qFatal("Fullscreen did not restore single host geometry");
                            preview->activateWindow();preview->showProjectionControls();
                            const QRect stablePhone=preview->phoneRect();
                            QTimer::singleShot(6000,&a,[&,stablePhone](){
                                auto *title=preview->findChild<QWidget*>("ProjectionTitle");auto *tools=preview->findChild<QWidget*>("ToolForm");
                                if(title->isVisible()||tools->isVisible()||preview->phoneRect()!=stablePhone||preview->mask()!=QRegion(stablePhone)){qCritical()<<"Idle state"<<title->isVisible()<<tools->isVisible()<<preview->phoneRect()<<stablePhone<<preview->property("chromeIdleMs")<<preview->property("chromeInteraction")<<preview->property("chromeMoved")<<preview->property("chromeCursor");qFatal("Idle hide retained hit area or changed phone layout");}
                                preview->showProjectionControls();
                                QWidget outside;QMouseEvent down(QEvent::MouseButtonPress,QPointF(1,1),QPointF(1,1),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&outside,&down);
                                if(title->isVisible()||tools->isVisible())qFatal("Outside click did not hide controls");
                                preview->showProjectionControls();preview->minimizeProjection();
                                if(title->isVisible()||tools->isVisible())qFatal("Minimize left child chrome visible");
                                QTimer::singleShot(350,&a,[&](){
                                    preview->showNormal();preview->activateWindow();preview->showProjectionControls();
                                    qInfo()<<"Single host: native ownership, transparent hit mask, stable phone layout, drag/click, corner/aspect, landscape/fullscreen, idle/outside hide, minimize passed";a.quit();
                                });
                            });
                        });
                    });
                });
            });
        });
    }

    int ret = a.exec();
    if(lifecycleSmoke&&(!g_mainDlg||!g_mainDlg->isShuttingDown()))qFatal("Application exited without shutdown cleanup");
    if (preview) delete preview;

    auto *finishedDialog = g_mainDlg;
    g_mainDlg = nullptr;
    delete finishedDialog;

#if defined(Q_OS_WIN32) || defined(Q_OS_MACOS)
    if (!isolatedMode) MouseTap::getInstance()->quitMouseEventTap();
#endif
    return ret;
}

void installTranslator()
{
    static QTranslator translator;
    QLocale locale;
    QLocale::Language language = locale.language();

    if (Config::getInstance().getLanguage() == "zh_CN") {
        language = QLocale::Chinese;
    } else if (Config::getInstance().getLanguage() == "en_US") {
        language = QLocale::English;
    } else if (Config::getInstance().getLanguage() == "ja_JP") {
        language = QLocale::Japanese;
    } else if (Config::getInstance().getLanguage() == "ko_KR") {
        language = QLocale::Korean;
    }

    QString languagePath = ":/i18n/";
    switch (language) {
    case QLocale::Chinese:
        languagePath += "zh_CN.qm";
        break;
    case QLocale::Japanese:
        languagePath += "ja_JP.qm";
        break;
    case QLocale::Korean:
        languagePath += "ko_KR.qm";
        break;
    case QLocale::English:
    default:
        languagePath += "en_US.qm";
        break;
    }

    auto loaded = translator.load(languagePath);
    if (!loaded) {
        qWarning() << "Failed to load translation file:" << languagePath;
    }
    qApp->installTranslator(&translator);
}

QtMsgType covertLogLevel(const QString &logLevel)
{
    if ("debug" == logLevel) {
        return QtDebugMsg;
    }

    if ("info" == logLevel) {
        return QtInfoMsg;
    }

    if ("warn" == logLevel) {
        return QtWarningMsg;
    }

    if ("error" == logLevel) {
        return QtCriticalMsg;
    }

#ifdef QT_NO_DEBUG
    return QtInfoMsg;
#else
    return QtDebugMsg;
#endif
}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString outputMsg;
    
#ifdef ENABLE_DETAILED_LOGS
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    
    if (context.file && context.line > 0) {
        QString fileName = QString::fromUtf8(context.file);

        int lastSlash = fileName.lastIndexOf('/');
        if (lastSlash >= 0) {
            fileName = fileName.mid(lastSlash + 1);
        }
        lastSlash = fileName.lastIndexOf('\\');
        if (lastSlash >= 0) {
            fileName = fileName.mid(lastSlash + 1);
        }
        
        outputMsg = QString("[ %1 %2: %3 ] %4").arg(timestamp).arg(fileName).arg(context.line).arg(msg);
    } else {
        outputMsg = QString("[%1] %2").arg(timestamp).arg(msg);
    }

    switch (type) {
    case QtDebugMsg:
        outputMsg.prepend("[debug] ");
        break;
    case QtInfoMsg:
        outputMsg.prepend("[info] ");
        break;
    case QtWarningMsg:
        outputMsg.prepend("[warring] ");
        break;
    case QtCriticalMsg:
        outputMsg.prepend("[critical] ");
        break;
    case QtFatalMsg:
        outputMsg.prepend("[fatal] ");
        break;
    }

    fprintf(stderr, "%s\n", outputMsg.toUtf8().constData());
#else
    outputMsg = msg;
    if (g_oldMessageHandler) {
        g_oldMessageHandler(type, context, outputMsg);
    }
#endif

    // Is Qt log level higher than warning?
    float fLogLevel = g_msgType;
    if (QtInfoMsg == g_msgType) {
        fLogLevel = QtDebugMsg + 0.5f;
    }
    float fLogLevel2 = type;
    if (QtInfoMsg == type) {
        fLogLevel2 = QtDebugMsg + 0.5f;
    }

    if (fLogLevel <= fLogLevel2) {
        if (g_mainDlg && g_mainDlg->isVisible() && !g_mainDlg->filterLog(outputMsg)) {
            g_mainDlg->outLog(outputMsg);
        }
    }

    if (QtFatalMsg == type) {
        //abort();
    }
}
