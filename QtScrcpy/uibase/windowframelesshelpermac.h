#ifndef WINDOWFRAMELESSHELPERMAC_H
#define WINDOWFRAMELESSHELPERMAC_H

#include <QQuickWindow>

class WindowFramelessHelperMac : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickWindow * target READ target WRITE setTarget NOTIFY targetChanged)

public:
    explicit WindowFramelessHelperMac(QObject *parent = nullptr);

    QQuickWindow *target() const;
    void setTarget(QQuickWindow *target);

signals:
    void targetChanged();

private:
    QQuickWindow* m_target = nullptr;
};

#endif // WINDOWFRAMELESSHELPERMAC_H
