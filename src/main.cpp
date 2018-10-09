#include "dialog.h"

#include <QApplication>
#include <QDebug>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qputenv("QTSCRCPY_ADB_PATH", "C:\\Users\\Barry\\Desktop\\scrcpy-win64\\adb.exe");

    Dialog w;
    w.show();

    return a.exec();
}
