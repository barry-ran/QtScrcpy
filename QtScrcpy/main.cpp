#include <QApplication>
#include <QDebug>
#include <QTcpSocket>
#include <QTcpServer>

#include "dialog.h"
#include "decoder.h"

int main(int argc, char *argv[])
{
    //QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    //QApplication::setAttribute(Qt::AA_UseOpenGLES);
    //QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);    

    Decoder::init();
    QApplication a(argc, argv);

#ifdef Q_OS_WIN32
    qputenv("QTSCRCPY_ADB_PATH", "..\\..\\..\\third_party\\adb\\adb.exe");
    qputenv("QTSCRCPY_SERVER_PATH", "..\\..\\..\\third_party\\scrcpy-server.jar");
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

    Dialog* w = new Dialog;
    w->show();

    int ret = a.exec();

    Decoder::deInit();
    return ret;
}
