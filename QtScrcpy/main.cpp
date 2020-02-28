#include <QApplication>
#include <QDebug>
#include <QTcpSocket>
#include <QTcpServer>
#include <QTranslator>
#include <QFile>
#include <QSurfaceFormat>

#include "dialog.h"
#include "stream.h"
#include "mousetap/mousetap.h"
#include "config.h"

static Dialog* g_mainDlg = Q_NULLPTR;

static QtMessageHandler g_oldMessageHandler = Q_NULLPTR;
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
void installTranslator();

int main(int argc, char *argv[])
{
    // set env
#ifdef Q_OS_WIN32
    qputenv("QTSCRCPY_ADB_PATH", "../../../../third_party/adb/win/adb.exe");
    qputenv("QTSCRCPY_SERVER_PATH", "../../../../third_party/scrcpy-server");
    qputenv("QTSCRCPY_KEYMAP_PATH", "../../../../keymap");
    qputenv("QTSCRCPY_CONFIG_PATH", "../../../../config");
#endif

#ifdef Q_OS_OSX
    qputenv("QTSCRCPY_KEYMAP_PATH", "../../../../../../keymap");
#endif

#ifdef Q_OS_LINUX
    qputenv("QTSCRCPY_ADB_PATH", "../../../third_party/adb/linux/adb");
    qputenv("QTSCRCPY_SERVER_PATH", "../../../third_party/scrcpy-server");
    qputenv("QTSCRCPY_CONFIG_PATH", "../../../config");
    qputenv("QTSCRCPY_KEYMAP_PATH", "../../../keymap");
#endif

    // set on QApplication before
    int opengl = Config::getInstance().getDesktopOpenGL();
    if (0 == opengl) {
        QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    } else if (1 == opengl){
        QApplication::setAttribute(Qt::AA_UseOpenGLES);
    } else if (2 == opengl) {
        QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    }

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

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
    Stream::init();
    QApplication a(argc, argv);

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
#if defined(Q_OS_WIN32) || defined(Q_OS_OSX)
    MouseTap::getInstance()->initMouseEventTap();
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

    g_mainDlg = new Dialog;
    g_mainDlg->setWindowTitle(Config::getInstance().getTitle());
    g_mainDlg->show();

    qInfo(QObject::tr("This software is completely open source and free, you can download it at the following address:").toUtf8());
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
    //language = QLocale::English;
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
        if (g_mainDlg && g_mainDlg->isVisible() && !g_mainDlg->filterLog(msg)) {
            g_mainDlg->outLog(msg);
        }
    }
    if (QtFatalMsg == type) {
        //abort();
    }
}
