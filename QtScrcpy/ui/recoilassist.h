#ifndef RECOILASSIST_H
#define RECOILASSIST_H

#include <QObject>
#include <QTimer>
#include <QPointer>

/**
 * @brief RecoilAssist - Auto vertical mouse compensation during firing.
 *
 * When fire key is held, this class emits periodic relative mouse movement
 * events to compensate upward recoil (moves mouse down to counteract).
 *
 * Usage: call setActive(true) on fire key press, setActive(false) on release.
 */
class RecoilAssist : public QObject
{
    Q_OBJECT

public:
    explicit RecoilAssist(QObject *parent = nullptr);

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // Strength: 0.0 (off) .. 1.0 (full). Default 0.3
    void setStrength(float strength);
    float strength() const { return m_strength; }

    // Interval ms between compensation ticks (default 16ms ~ 60fps)
    void setInterval(int ms);

    // Call these when fire is pressed/released
    void onFirePressed();
    void onFireReleased();

signals:
    // deltaX/deltaY are in screen pixels to move the mouse
    void compensationMove(int deltaX, int deltaY);

private slots:
    void onTick();

private:
    bool  m_enabled  = false;
    bool  m_firing   = false;
    float m_strength = 0.30f;
    int   m_tickCount = 0;
    QTimer m_timer;
};

#endif // RECOILASSIST_H