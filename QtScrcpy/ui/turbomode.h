#ifndef TURBOMODE_H
#define TURBOMODE_H

#include <QObject>
#include <QString>

/**
 * @brief TurboMode - Temporarily overrides bitrate and maxFps for peak performance.
 *
 * Call activate() to enter turbo mode (8K bitrate, 120fps cap).
 * Call deactivate() to restore previous settings.
 * Turbo mode auto-reverts after timeoutSec seconds (default 0 = never).
 */
class TurboMode : public QObject
{
    Q_OBJECT

public:
    explicit TurboMode(QObject *parent = nullptr);

    void setSerial(const QString &serial);

    bool isActive() const { return m_active; }

    // Call this to flip turbo on/off
    void toggle();

    // Turbo parameters (can be customised from Settings)
    void setTurboParams(quint32 bitrate, quint32 maxFps);

signals:
    void turboActivated();
    void turboDeactivated();
    // Notify the UI to show a toast
    void toastMessage(const QString &msg);

private:
    void activate();
    void deactivate();

    bool    m_active    = false;
    QString m_serial;
    quint32 m_turboBitrate = 16000000; // 16 Mbps
    quint32 m_turboFps     = 120;
};

#endif // TURBOMODE_H