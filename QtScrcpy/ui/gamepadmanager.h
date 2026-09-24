#pragma once
#include <QObject>
#include <QTimer>
#include <QMap>
#include <QString>
#include <QPointF>

/**
 * GamepadManager - Windows XInput polling bridge.
 * Maps gamepad buttons/sticks to virtual touch events on the Android device.
 * Compatible with: Xbox 360/One/Series, DualShock4 via DS4Windows,
 *                  DualSense via DualSenseX.
 */
class GamepadManager : public QObject
{
    Q_OBJECT
public:
    struct ButtonMapping {
        float  touchX = 0.5f;   // screen ratio [0,1]
        float  touchY = 0.5f;
        QString label;
    };

    explicit GamepadManager(const QString &serial, QObject *parent = nullptr);
    ~GamepadManager() override;

    void setEnabled(bool en);
    bool isEnabled()   const { return m_enabled;   }
    bool isConnected() const { return m_connected; }

    void setButtonMapping(unsigned short xinputButton, ButtonMapping map);
    void setLeftStickSensitivity(float s)  { m_leftSens  = s; }
    void setRightStickSensitivity(float s) { m_rightSens = s; }
    void setWasdCenter(float x, float y)   { m_wasdCX = x; m_wasdCY = y; }
    void setAimCenter(float x, float y)    { m_aimCX  = x; m_aimCY  = y; }

signals:
    void connectedChanged(bool connected);
    void statusMessage(const QString &msg);

private slots:
    void poll();

private:
    void sendTouch(int id, float rx, float ry, bool down);
    void sendTouchMove(int id, float rx, float ry);
    void processButtons(unsigned short current, unsigned short prev);
    void processLeftStick(short lx, short ly);
    void processRightStick(short rx, short ry);

    QString   m_serial;
    QTimer   *m_timer     = nullptr;
    bool      m_enabled   = false;
    bool      m_connected = false;

    float m_leftSens  = 1.0f;
    float m_rightSens = 1.0f;
    float m_wasdCX = 0.25f, m_wasdCY = 0.70f;
    float m_aimCX  = 0.60f, m_aimCY  = 0.40f;

    unsigned short m_prevButtons      = 0;
    short          m_prevLX = 0, m_prevLY = 0;
    short          m_prevRX = 0, m_prevRY = 0;
    bool           m_leftStickActive  = false;
    bool           m_rightStickActive = false;
    float          m_aimAccX = 0.f, m_aimAccY = 0.f;

    // Touch IDs 20-29 reserved for gamepad
    static constexpr int kTouchIdBase = 20;

    QMap<unsigned short, ButtonMapping> m_buttonMap;
    QMap<unsigned short, bool>          m_buttonDown;
};