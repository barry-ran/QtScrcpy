#ifndef MOUSETAP_H
#define MOUSETAP_H
class QWidget;
class MouseTap {
public:
    static MouseTap* getInstance();
    virtual void initMouseEventTap() = 0;
    virtual void quitMouseEventTap() = 0;
    virtual void enableMouseEventTap(QWidget* widget, bool enabled) = 0;

private:
    static MouseTap *s_instance;
};
#endif // MOUSETAP_H
