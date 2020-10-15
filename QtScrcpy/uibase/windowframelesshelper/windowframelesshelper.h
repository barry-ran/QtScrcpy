#pragma once

#include <QQuickWindow>

class WindowFramelessHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickWindow * target READ target WRITE setTarget NOTIFY targetChanged)

public:
    explicit WindowFramelessHelper(QObject *parent = nullptr);
    virtual ~WindowFramelessHelper();

    QQuickWindow *target() const;
    void setTarget(QQuickWindow *target);

signals:
    void targetChanged();

private:
    QQuickWindow* m_target = nullptr;
};
