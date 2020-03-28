#ifndef ICONHELPER_H
#define ICONHELPER_H

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QLabel>
#include <QMutex>
#include <QObject>
#include <QPushButton>

class IconHelper : public QObject
{
private:
    explicit IconHelper(QObject *parent = 0);
    QFont iconFont;
    static IconHelper *_instance;

public:
    static IconHelper *Instance()
    {
        static QMutex mutex;
        if (!_instance) {
            QMutexLocker locker(&mutex);
            if (!_instance) {
                _instance = new IconHelper;
            }
        }
        return _instance;
    }

    void SetIcon(QLabel *lab, QChar c, int size = 10);
    void SetIcon(QPushButton *btn, QChar c, int size = 10);
};

#endif // ICONHELPER_H
