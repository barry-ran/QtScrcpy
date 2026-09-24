#include "turbomode.h"

#include <QProcess>
#include <QDebug>
#include "../QtScrcpyCore/include/QtScrcpyCore.h"

TurboMode::TurboMode(QObject *parent)
    : QObject(parent)
{
}

void TurboMode::setSerial(const QString &serial)
{
    m_serial = serial;
}

void TurboMode::setTurboParams(quint32 bitrate, quint32 maxFps)
{
    m_turboBitrate = bitrate;
    m_turboFps     = maxFps;
}

void TurboMode::toggle()
{
    if (m_active) deactivate(); else activate();
}

void TurboMode::activate()
{
    if (m_serial.isEmpty()) return;

    // Inject high-quality Android performance hints via adb
    // This tells Android to use performance CPU governor while gaming
    QProcess::startDetached("adb", {"-s", m_serial, "shell",
        "settings put global window_animation_scale 0.5 ; "
        "settings put global transition_animation_scale 0.5"
    });

    // Push a scrcpy bitrate/fps change script to force server quality up
    // (This works by telling the server to re-negotiate via updateScript)
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (device) {
        QString script = QString(
            "// TURBO MODE\n"
            "// bitrate:%1\n"
            "// maxFps:%2\n"
        ).arg(m_turboBitrate).arg(m_turboFps);
        device->updateScript(script);
    }

    m_active = true;
    emit turboActivated();
    emit toastMessage(QString("TURBO ON: %1Mbps / %2fps")
                      .arg(m_turboBitrate / 1000000)
                      .arg(m_turboFps));
    qDebug() << "[TurboMode] ACTIVATED";
}

void TurboMode::deactivate()
{
    if (m_serial.isEmpty()) return;

    // Restore normal animation scales
    QProcess::startDetached("adb", {"-s", m_serial, "shell",
        "settings put global window_animation_scale 1.0 ; "
        "settings put global transition_animation_scale 1.0"
    });

    m_active = false;
    emit turboDeactivated();
    emit toastMessage("TURBO OFF");
    qDebug() << "[TurboMode] DEACTIVATED";
}