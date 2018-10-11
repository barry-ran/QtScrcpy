#include "dialog.h"

#include <QApplication>
#include <QDebug>
#include <QTcpSocket>
#include <QTcpServer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qputenv("QTSCRCPY_ADB_PATH", "C:\\Users\\Barry\\Desktop\\scrcpy-win64\\adb.exe");
    qputenv("QTSCRCPY_SERVER_PATH", "G:\\mygitcode\\QtScrcpy\\src\\scrcpy-server.jar");

    Dialog w;
    w.show();

    return a.exec();
}
