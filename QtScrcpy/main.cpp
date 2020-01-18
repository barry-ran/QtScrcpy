#include <QApplication>
#include <QDebug>
#include <QTcpSocket>
#include <QTcpServer>
#include <QTranslator>
#include <QFile>

#include "dialog.h"
#include "stream.h"
#include "mousetap/mousetap.h"
#include "config.h"

Dialog* g_mainDlg = Q_NULLPTR;

QtMessageHandler g_oldMessageHandler = Q_NULLPTR;
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
void installTranslator();

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    g_oldMessageHandler = qInstallMessageHandler(myMessageOutput);
    Stream::init();
    QApplication a(argc, argv);

    // windows下通过qmake VERSION变量或者rc设置版本号和应用名称后，这里可以直接拿到
    // mac下拿到的是CFBundleVersion的值
    qDebug() << a.applicationVersion();
    qDebug() << a.applicationName();

    //update version
    QStringList versionList = QCoreApplication::applicationVersion().split(".");
    QString version = versionList[0] + "." + versionList[1] + "." + versionList[2];
    a.setApplicationVersion(version);

    installTranslator();
#if defined(Q_OS_WIN32) || defined(Q_OS_OSX)
    MouseTap::getInstance()->initMouseEventTap();
#endif

#ifdef Q_OS_WIN32
    qputenv("QTSCRCPY_ADB_PATH", "../../../../third_party/adb/win/adb.exe");
    qputenv("QTSCRCPY_SERVER_PATH", "../../../../third_party/scrcpy-server");
    qputenv("QTSCRCPY_KEYMAP_PATH", "../../../../keymap");
    qputenv("QTSCRCPY_CONFIG_PATH", "../../../../config/config.ini");
#endif

#ifdef Q_OS_LINUX
    qputenv("QTSCRCPY_ADB_PATH", "../../../third_party/adb/linux/adb");
    qputenv("QTSCRCPY_SERVER_PATH", "../../../third_party/scrcpy-server");
    qputenv("QTSCRCPY_CONFIG_PATH", "../../../config/config.ini");
#endif

    //加载样式表
    QFile file(":/qss/psblack.css");
    if (file.open(QFile::ReadOnly)) {
        QString qss = QLatin1String(file.readAll());
        QString paletteColor = qss.mid(20, 7);
        qApp->setPalette(QPalette(QColor(paletteColor)));
        qApp->setStyleSheet(qss);
        file.close();
    }

    int opengl = Config::getInstance().getDesktopOpenGL();
    if (0 == opengl) {
        QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    } else if (1 == opengl){
        QApplication::setAttribute(Qt::AA_UseOpenGLES);
    } else if (2 == opengl) {
        QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    }

    g_mainDlg = new Dialog;
    g_mainDlg->setWindowTitle(Config::getInstance().getTitle());
    g_mainDlg->show();

    qInfo(QString("QtScrcpy %1 <https://github.com/barry-ran/QtScrcpy>").arg(QCoreApplication::applicationVersion()).toUtf8());

    int ret = a.exec();

#if defined(Q_OS_WIN32) || defined(Q_OS_OSX)
    MouseTap::getInstance()->quitMouseEventTap();
#endif

    Stream::deInit();
    return ret;
}

void installTranslator() {
    static QTranslator translator;
    QLocale locale;
    QLocale::Language language = locale.language();
    QString languagePath = ":/i18n/";
    switch (language) {
    case QLocale::Chinese:
        languagePath += "QtScrcpy_zh.qm";
        break;
    case QLocale::English:
    default:
        languagePath += "QtScrcpy_en.qm";
    }

    translator.load(languagePath);
    qApp->installTranslator(&translator);
}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (g_oldMessageHandler) {
        g_oldMessageHandler(type, context, msg);
    }

    if (QtDebugMsg < type) {
        if (g_mainDlg && g_mainDlg->isVisible() && !msg.contains("app_proces")) {
            g_mainDlg->outLog(msg);
        }
    }
    if (QtFatalMsg == type) {
        //abort();
    }
}
