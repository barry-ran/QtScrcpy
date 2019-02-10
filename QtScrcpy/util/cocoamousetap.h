#ifndef COCOAMOUSETAP_H
#define COCOAMOUSETAP_H
#include <QThread>
#include <QSemaphore>

struct MouseEventTapData;
class QWidget;
class CocoaMouseTap : public QThread
{
private:
    CocoaMouseTap(QObject *parent = Q_NULLPTR);
    ~CocoaMouseTap();

public:
    static CocoaMouseTap* getInstance();
    void initMouseEventTap();
    void quitMouseEventTap();
    void enableMouseEventTap(QWidget* widget, bool enabled);

protected:
    void run() override;

private:
    MouseEventTapData *m_tapData = Q_NULLPTR;
    QSemaphore m_runloopStartedSemaphore;
    static CocoaMouseTap *s_instance;
};

#endif // COCOAMOUSETAP_H
