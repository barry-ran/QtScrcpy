#include <QApplication>
#include <QDebug>
#include <QTcpSocket>
#include <QTcpServer>
#include <QTranslator>

#include "dialog.h"
#include "decoder.h"

Dialog* g_mainDlg = Q_NULLPTR;

QtMessageHandler g_oldMessageHandler = Q_NULLPTR;
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
void installTranslator();

int main(int argc, char *argv[])
{
    //QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    //QApplication::setAttribute(Qt::AA_UseOpenGLES);
    //QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);

    g_oldMessageHandler = qInstallMessageHandler(myMessageOutput);
    Decoder::init();
    QApplication a(argc, argv);

    installTranslator();

#ifdef Q_OS_WIN32
    qputenv("QTSCRCPY_ADB_PATH", "../../../third_party/adb/win/adb.exe");
    qputenv("QTSCRCPY_SERVER_PATH", "../../../third_party/scrcpy-server.jar");
#endif

#ifdef Q_OS_LINUX
    qputenv("QTSCRCPY_ADB_PATH", "../../../third_party/adb/linux/adb");
    qputenv("QTSCRCPY_SERVER_PATH", "../../../third_party/scrcpy-server.jar");
#endif

    //加载样式表
    QFile file(":/res/psblack.css");
    if (file.open(QFile::ReadOnly)) {
        QString qss = QLatin1String(file.readAll());
        QString paletteColor = qss.mid(20, 7);
        qApp->setPalette(QPalette(QColor(paletteColor)));
        qApp->setStyleSheet(qss);
        file.close();
    }

    g_mainDlg = new Dialog;
    g_mainDlg->show();

    int ret = a.exec();

    Decoder::deInit();
    return ret;
}

void installTranslator() {
    static QTranslator translator;
    QLocale locale;
    QLocale::Language language = locale.language();
    QString languagePath = ":i18n/";
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
        if (g_mainDlg && !msg.contains("app_proces")) {
            g_mainDlg->outLog(msg);
        }
    }
    if (QtFatalMsg == type) {
        //abort();
    }
}
