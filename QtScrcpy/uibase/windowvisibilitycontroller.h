#ifndef WINDOWVISIBILITYCONTROLLER_H
#define WINDOWVISIBILITYCONTROLLER_H

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QScopedPointer>
#include <QTimer>

class QWidget;

// Keeps the video, its independent tool window and owned popups together.
// A nearly transparent window still receives mouse hits, so moving back over
// its original location can reveal it without changing focus or stacking.
class WindowVisibilityController : public QObject
{
    Q_OBJECT

public:
    explicit WindowVisibilityController(QWidget *host);
    ~WindowVisibilityController() override;

    void addWindow(QWidget *window);
    void setOpacityPercent(int percent);
    int opacityPercent() const;
    void setAutoHideEnabled(bool enabled);
    bool autoHideEnabled() const;

signals:
    void opacityPercentChanged(int percent);
    void autoHideEnabledChanged(bool enabled);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    struct NativeState;
    bool belongsToGroup(QWidget *widget) const;
    QList<QWidget *> groupWindows() const;
    bool pointerIsNear(bool includeMargin) const;
    bool interactionInProgress() const;
    void applyOpacity();
    void applyOpacity(QWidget *window);
    void resetVisibility();
    void tick();

    QPointer<QWidget> m_host;
    QList<QPointer<QWidget>> m_windows;
    QList<QPointer<QWidget>> m_appliedWindows;
    QPointer<QWidget> m_composingWidget;
    QScopedPointer<NativeState> m_native;
    QElapsedTimer m_clock;
    QTimer m_timer;
    qint64 m_lastNear = 0;
    qint64 m_revealStarted = -1;
    int m_opacityPercent = 100;
    bool m_autoHideEnabled = false;
    bool m_autoHidden = false;
    bool m_applying = false;
};

#endif // WINDOWVISIBILITYCONTROLLER_H
