#include "deviceinfooverlay.h"

#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QFont>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QDebug>
#include <cmath>

// ============================================================================
DeviceInfoOverlay::DeviceInfoOverlay(const QString &serial, QWidget *parent)
    : QWidget(parent), m_serial(serial)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setWindowFlags(Qt::FramelessWindowHint);
    setFixedSize(120, 44);

    m_pollTimer.setInterval(10000);  // poll every 10 seconds
    connect(&m_pollTimer, &QTimer::timeout, this, &DeviceInfoOverlay::onPollTimer);
}

DeviceInfoOverlay::~DeviceInfoOverlay()
{
    m_pollTimer.stop();
}

// ============================================================================
void DeviceInfoOverlay::startPolling()
{
    // Fetch immediately on start
    onPollTimer();
    m_pollTimer.start();
    repositionSelf();
    show();
    raise();
}

void DeviceInfoOverlay::stopPolling()
{
    m_pollTimer.stop();
    hide();
}

void DeviceInfoOverlay::setCorner(Qt::Corner corner)
{
    m_corner = corner;
    repositionSelf();
}

// ============================================================================
void DeviceInfoOverlay::repositionSelf()
{
    if (!parentWidget()) return;
    QSize ps = parentWidget()->size();
    int margin = 6;
    int x = 0, y = 0;
    switch (m_corner) {
    case Qt::TopLeftCorner:     x = margin;                           y = margin; break;
    case Qt::TopRightCorner:    x = ps.width() - width() - margin;   y = margin; break;
    case Qt::BottomLeftCorner:  x = margin;                           y = ps.height() - height() - margin; break;
    case Qt::BottomRightCorner: x = ps.width() - width() - margin;   y = ps.height() - height() - margin; break;
    }
    move(x, y);
}

// ============================================================================
void DeviceInfoOverlay::onPollTimer()
{
    fetchBattery();
    fetchTemp();
}

void DeviceInfoOverlay::fetchBattery()
{
    // adb -s <serial> shell dumpsys battery | grep level
    QProcess *proc = new QProcess(this);
    QStringList args;
    args << "-s" << m_serial << "shell" << "dumpsys" << "battery";
    connect(proc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this, proc](int, QProcess::ExitStatus) {
        QString out = proc->readAllStandardOutput();
        proc->deleteLater();
        for (const QString &line : out.split('\n')) {
            if (line.contains("level:")) {
                bool ok;
                int v = line.mid(line.indexOf(':') + 1).trimmed().toInt(&ok);
                if (ok) {
                    m_battery = v;
                    update();
                }
                break;
            }
        }
    });
    proc->start("adb", args);
}

void DeviceInfoOverlay::fetchTemp()
{
    // adb -s <serial> shell cat /sys/class/thermal/thermal_zone0/temp
    QProcess *proc = new QProcess(this);
    QStringList args;
    args << "-s" << m_serial << "shell" << "cat" << "/sys/class/thermal/thermal_zone0/temp";
    connect(proc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this, proc](int, QProcess::ExitStatus) {
        QString out = proc->readAllStandardOutput().trimmed();
        proc->deleteLater();
        bool ok;
        int raw = out.toInt(&ok);
        if (ok && raw > 0) {
            // Most devices report millidegrees
            m_tempC = (raw > 1000) ? (raw / 1000.f) : raw;
            update();
        }
    });
    proc->start("adb", args);
}

// ============================================================================
void DeviceInfoOverlay::paintEvent(QPaintEvent *)
{
    if (m_battery < 0 && m_tempC < 0) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Pill background
    QRectF bg = rect().adjusted(2, 2, -2, -2);
    QPainterPath path;
    path.addRoundedRect(bg, 10, 10);
    p.fillPath(path, QColor(0, 0, 0, 160));

    p.setPen(Qt::white);

    // Icon + Battery
    if (m_battery >= 0) {
        QString batStr;
        QColor batColor;
        if (m_battery >= 80)        { batStr = QString(u8"\U0001F50B %1%%").arg(m_battery); batColor = QColor("#22c55e"); }
        else if (m_battery >= 40)   { batStr = QString(u8"\U0001F50B %1%%").arg(m_battery); batColor = QColor("#fbbf24"); }
        else                        { batStr = QString(u8"\U0001F50B %1%%").arg(m_battery); batColor = QColor("#ef4444"); }

        QFont fnt = p.font();
        fnt.setPointSize(9);
        fnt.setBold(true);
        p.setFont(fnt);
        p.setPen(batColor);
        p.drawText(QRectF(6, 4, width() - 8, 18), Qt::AlignLeft | Qt::AlignVCenter, batStr);
    }

    // Temperature
    if (m_tempC >= 0) {
        QColor tColor = (m_tempC > 45.f) ? QColor("#ef4444") : (m_tempC > 38.f) ? QColor("#fbbf24") : QColor("#94a3b8");
        QString tempStr = QString(u8"\U0001F321 %1\u00B0C").arg(m_tempC, 0, 'f', 1);
        QFont fnt = p.font();
        fnt.setPointSize(9);
        fnt.setBold(true);
        p.setFont(fnt);
        p.setPen(tColor);
        p.drawText(QRectF(6, 22, width() - 8, 18), Qt::AlignLeft | Qt::AlignVCenter, tempStr);
    }
}