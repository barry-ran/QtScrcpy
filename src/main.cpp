#include "dialog.h"

#include <QApplication>
#include <QDebug>
#include <QTcpSocket>
#include <QTcpServer>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    //QApplication::setAttribute(Qt::AA_UseOpenGLES);
    //QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);

    QApplication a(argc, argv);

    qputenv("QTSCRCPY_ADB_PATH", "G:\\mygitcode\\QtScrcpy\\src\\adb.exe");
    qputenv("QTSCRCPY_SERVER_PATH", "G:\\mygitcode\\QtScrcpy\\src\\scrcpy-server.jar");

    Dialog w;
    w.show();

    return a.exec();
}
