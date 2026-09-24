#include "gamepadmanager.h"
#include "../../QtScrcpyCore/include/QtScrcpyCore.h"

#include <QDebug>

#if defined(Q_OS_WIN)
#include <Windows.h>
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")
#endif

// ─── XInput button bitmasks ───────────────────────────────────────────────────
#define GP_DPAD_UP      0x0001
#define GP_DPAD_DOWN    0x0002
#define GP_DPAD_LEFT    0x0004
#define GP_DPAD_RIGHT   0x0008
#define GP_START        0x0010
#define GP_BACK         0x0020
#define GP_LTHUMB       0x0040
#define GP_RTHUMB       0x0080
#define GP_LB           0x0100
#define GP_RB           0x0200
#define GP_A            0x1000
#define GP_B            0x2000
#define GP_X            0x4000
#define GP_Y            0x8000

static constexpr short kDeadZone = 8000;

// ─── Constructor ──────────────────────────────────────────────────────────────
GamepadManager::GamepadManager(const QString &serial, QObject *parent)
    : QObject(parent), m_serial(serial)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(16); // ~60 fps polling
    connect(m_timer, &QTimer::timeout, this, &GamepadManager::poll);

    // Default button mappings – can be overridden per-profile
    m_buttonMap[GP_A]  = {0.50f, 0.85f, "A (Jump)"};
    m_buttonMap[GP_B]  = {0.85f, 0.50f, "B (Crouch)"};
    m_buttonMap[GP_X]  = {0.20f, 0.70f, "X (Reload)"};
    m_buttonMap[GP_Y]  = {0.80f, 0.20f, "Y (Grenade)"};
    m_buttonMap[GP_LB] = {0.10f, 0.55f, "LB (Prone)"};
    m_buttonMap[GP_RB] = {0.90f, 0.35f, "RB (Scope)"};
    m_buttonMap[GP_LB] = {0.10f, 0.55f, "LB (Prone)"};
    m_buttonMap[GP_DPAD_UP]    = {0.50f, 0.15f, "DPad Up (Map)"};
    m_buttonMap[GP_DPAD_DOWN]  = {0.50f, 0.85f, "DPad Down (Bag)"};
    m_buttonMap[GP_DPAD_LEFT]  = {0.15f, 0.50f, "DPad Left"};
    m_buttonMap[GP_DPAD_RIGHT] = {0.85f, 0.50f, "DPad Right"};
    m_buttonMap[GP_START] = {0.95f, 0.05f, "Start (Menu)"};
}

GamepadManager::~GamepadManager()
{
    setEnabled(false);
}

void GamepadManager::setEnabled(bool en)
{
    if (m_enabled == en) return;
    m_enabled = en;
    if (en) {
        m_prevButtons      = 0;
        m_prevLX = m_prevLY = 0;
        m_prevRX = m_prevRY = 0;
        m_leftStickActive  = false;
        m_rightStickActive = false;
        m_aimAccX = m_aimAccY = 0.f;
        m_timer->start();
        qInfo() << "[Gamepad] Polling started";
    } else {
        m_timer->stop();
        qInfo() << "[Gamepad] Polling stopped";
    }
}

void GamepadManager::setButtonMapping(unsigned short xinputButton, ButtonMapping map)
{
    m_buttonMap[xinputButton] = map;
}

// ─── Main poll loop ───────────────────────────────────────────────────────────
void GamepadManager::poll()
{
#if !defined(Q_OS_WIN)
    return; // XInput is Windows-only
#else
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(state));

    DWORD result = XInputGetState(0, &state); // User index 0 (first controller)
    bool wasConnected = m_connected;
    m_connected = (result == ERROR_SUCCESS);

    if (m_connected != wasConnected) {
        emit connectedChanged(m_connected);
        if (m_connected) {
            emit statusMessage(tr("🎮 Gamepad connected!"));
        } else {
            emit statusMessage(tr("🎮 Gamepad disconnected"));
        }
    }

    if (!m_connected) return;

    auto &gp = state.Gamepad;
    unsigned short curButtons = gp.wButtons;

    processButtons(curButtons, m_prevButtons);
    processLeftStick(gp.sThumbLX, gp.sThumbLY);
    processRightStick(gp.sThumbRX, gp.sThumbRY);

    m_prevButtons = curButtons;
    m_prevLX = gp.sThumbLX;
    m_prevLY = gp.sThumbLY;
    m_prevRX = gp.sThumbRX;
    m_prevRY = gp.sThumbRY;
#endif
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
void GamepadManager::sendTouch(int id, float rx, float ry, bool down)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) return;

    // We synthesise a QMouseEvent and pass it through the normal device path
    // using a custom approach: we directly create a fake QMouseEvent at the
    // relative position on the video widget. Since we don't have easy access to
    // the VideoForm size here, we use the stored frame/show size from the
    // conversion layer via the normal mouseEvent() pipeline.
    //
    // The trick: Qt::ExtraButton1..24 are mapped to touch IDs inside
    // InputConvertGame. We use ExtraButton1 + offset.
    //
    // NOTE: The device->mouseEvent() path scales by showSize inside the core.
    // We pass a showSize of (1000,1000) and absolute coords in [0,1000].
    QSize fakeSz(1000, 1000);
    QPoint pt(static_cast<int>(rx * 1000.f), static_cast<int>(ry * 1000.f));

    Qt::MouseButton btn = static_cast<Qt::MouseButton>(Qt::ExtraButton1 << (id - kTouchIdBase));
    QMouseEvent ev(
        down ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease,
        QPointF(pt), QPointF(pt),
        btn, down ? btn : Qt::NoButton,
        Qt::NoModifier
    );
    device->mouseEvent(&ev, fakeSz, fakeSz);
}

void GamepadManager::sendTouchMove(int id, float rx, float ry)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) return;

    QSize fakeSz(1000, 1000);
    QPoint pt(static_cast<int>(rx * 1000.f), static_cast<int>(ry * 1000.f));

    Qt::MouseButton btn = static_cast<Qt::MouseButton>(Qt::ExtraButton1 << (id - kTouchIdBase));
    QMouseEvent ev(
        QEvent::MouseMove,
        QPointF(pt), QPointF(pt),
        Qt::NoButton, btn,
        Qt::NoModifier
    );
    device->mouseEvent(&ev, fakeSz, fakeSz);
}

void GamepadManager::processButtons(unsigned short current, unsigned short prev)
{
    for (auto it = m_buttonMap.begin(); it != m_buttonMap.end(); ++it) {
        unsigned short mask = it.key();
        bool isDown = (current & mask) != 0;
        bool wasDown = (prev & mask) != 0;
        int touchId = kTouchIdBase + 5 + (int)std::distance(m_buttonMap.begin(), it);

        if (isDown && !wasDown) {
            sendTouch(touchId, it->touchX, it->touchY, true);
            m_buttonDown[mask] = true;
        } else if (!isDown && wasDown) {
            sendTouch(touchId, it->touchX, it->touchY, false);
            m_buttonDown[mask] = false;
        }
    }
}

void GamepadManager::processLeftStick(short lx, short ly)
{
    bool active = (abs(lx) > kDeadZone || abs(ly) > kDeadZone);

    if (!active && m_leftStickActive) {
        // release touch - return to center
        sendTouch(kTouchIdBase, m_wasdCX, m_wasdCY, false);
        m_leftStickActive = false;
        return;
    }

    if (!active) return;

    // Normalise [-1, 1]
    float nx = static_cast<float>(lx) / 32767.f;
    float ny = -static_cast<float>(ly) / 32767.f; // Y is inverted in XInput

    // Clamp to circle of radius 0.12 around wasdCenter
    float radius = 0.12f * m_leftSens;
    float tx = m_wasdCX + nx * radius;
    float ty = m_wasdCY + ny * radius;
    tx = qBound(0.f, tx, 1.f);
    ty = qBound(0.f, ty, 1.f);

    if (!m_leftStickActive) {
        sendTouch(kTouchIdBase, m_wasdCX, m_wasdCY, true);
        m_leftStickActive = true;
    }
    sendTouchMove(kTouchIdBase, tx, ty);
}

void GamepadManager::processRightStick(short rx, short ry)
{
    bool active = (abs(rx) > kDeadZone || abs(ry) > kDeadZone);

    if (!active) {
        m_rightStickActive = false;
        m_aimAccX = m_aimCX;
        m_aimAccY = m_aimCY;
        return;
    }

    float nx = static_cast<float>(rx) / 32767.f;
    float ny = -static_cast<float>(ry) / 32767.f;

    float speed = 0.003f * m_rightSens;
    m_aimAccX = qBound(0.05f, m_aimAccX + nx * speed * 50, 0.95f);
    m_aimAccY = qBound(0.05f, m_aimAccY + ny * speed * 50, 0.95f);

    if (!m_rightStickActive) {
        sendTouch(kTouchIdBase + 1, m_aimCX, m_aimCY, true);
        m_rightStickActive = true;
    }
    sendTouchMove(kTouchIdBase + 1, m_aimAccX, m_aimAccY);
}