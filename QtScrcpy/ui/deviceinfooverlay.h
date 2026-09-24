#ifndef DEVICEINFOOVERLAY_H
#define DEVICEINFOOVERLAY_H

#include <QWidget>
#include <QTimer>
#include <QPointer>
#include <QProcess>

/**
 * @brief Transparent overlay showing battery % and CPU temperature.
 * Polls the device via ADB every 10 seconds.
 * Place as a child of VideoForm and call startPolling().
 */
class DeviceInfoOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceInfoOverlay(const QString &serial, QWidget *parent = nullptr);
    ~DeviceInfoOverlay() override;

    void startPolling();
    void stopPolling();
    void setCorner(Qt::Corner corner);

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void onPollTimer();

private:
    void fetchBattery();
    void fetchTemp();
    void repositionSelf();

    QString    m_serial;
    QTimer     m_pollTimer;
    int        m_battery  = -1;
    float      m_tempC    = -1.f;
    Qt::Corner m_corner   = Qt::TopRightCorner;
};

#endif // DEVICEINFOOVERLAY_H