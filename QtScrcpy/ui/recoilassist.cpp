#include "recoilassist.h"
#include <cmath>

RecoilAssist::RecoilAssist(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, &RecoilAssist::onTick);
}

void RecoilAssist::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled) {
        m_timer.stop();
        m_firing = false;
        m_tickCount = 0;
    }
}

void RecoilAssist::setStrength(float strength)
{
    m_strength = qBound(0.f, strength, 1.f);
}

void RecoilAssist::setInterval(int ms)
{
    m_timer.setInterval(qBound(8, ms, 100));
}

void RecoilAssist::onFirePressed()
{
    if (!m_enabled) return;
    m_firing = true;
    m_tickCount = 0;
    m_timer.start();
}

void RecoilAssist::onFireReleased()
{
    m_firing = false;
    m_timer.stop();
    m_tickCount = 0;
}

void RecoilAssist::onTick()
{
    if (!m_firing) return;

    // Recoil pattern: starts strong, tapers slightly over time
    // Positive Y = move mouse DOWN (fighting upward gun kick)
    float base = 4.0f * m_strength;

    // Slight horizontal drift randomness (like real spray patterns)
    // pattern index-based: alternating left/right small amount
    float driftX = (m_tickCount % 4 < 2) ? (0.5f * m_strength) : (-0.5f * m_strength);

    // Taper: after ~20 ticks reduce compensation (barrel stabilizes)
    float taper = (m_tickCount < 20) ? 1.0f : qMax(0.4f, 1.0f - (m_tickCount - 20) * 0.02f);

    int dy = static_cast<int>(std::round(base * taper));
    int dx = static_cast<int>(std::round(driftX));

    if (dy > 0 || dx != 0) {
        emit compensationMove(dx, dy);
    }

    ++m_tickCount;
}