#include "encoderpreset.h"

#include <QCoreApplication>

namespace {

// Builds the MediaTek encoder preset. The strings are marked for translation
// with a stable "PresetConfigDialog" context so lupdate keeps collecting them
// regardless of which file they live in.
EncoderPreset makeMtkPreset()
{
    auto tr = [](const char *sourceText) {
        return QCoreApplication::translate("PresetConfigDialog", sourceText);
    };

    EncoderPreset preset;
    preset.id = QStringLiteral("mtk");
    preset.displayName = tr("MTK Encoder");
    preset.codecName = QStringLiteral("c2.mtk.avc.encoder");
    preset.warning = tr(
        "This feature is for MediaTek (MTK) devices only.\n"
        "Using on non-MTK devices may cause encoding failure or visual artifacts.");

    // All 3 levels use VBR (bitrate-mode=1) — CBR is not supported on
    // c2.mtk.avc.encoder (confirmed on MT6895, Android 13).
    //
    // Level 0 "Game Mode": tight QP ceiling (28) + short GOP (5)
    //   The hard QP cap physically prevents Sticky QP from ever degrading
    //   the frame. Even if the encoder's rate control tries to raise QP,
    //   it cannot exceed 28 — the frame stays sharp.
    static const QString gameMode = QStringLiteral(
        "profile=1,max-bframes=0,i-frame-interval=5,priority=0,bitrate-mode=1,"
        "video-qp-min=15,video-qp-max=28");

    // Level 1 "Balanced" / Level 2 "Power Saver": standard QP cap (35)
    static const QString common = QStringLiteral(
        "profile=1,max-bframes=0,i-frame-interval=10,priority=0,bitrate-mode=1,"
        "video-qp-max=35,"
        "vendor.mtk.ext.venc.i.frame.control.size.max-i-ratio:int=30,"
        "vendor.mtk.ext.venc.mbrc.tracking-speed:int=0");

    // Level 2 extras: disable visual optimizations, low-power ME, single ref frame
    static const QString visualOpt = QStringLiteral(
        "vendor.mtk.ext.venc.visual.rd:int=0,vendor.mtk.ext.venc.visual.quant:int=0");
    static const QString meLowPower = QStringLiteral(
        "vendor.mtk.ext.venc.highquality.feature-on:int=1,"
        "vendor.mtk.ext.venc.highquality.mode:int=0");
    static const QString extreme = QStringLiteral(
        "vendor.mtk.ext.venc.ref.frame.num:int=1,"
        "vendor.mtk.venc.dynamic.qpbound.min:int=28,"
        "vendor.mtk.venc.dynamic.qpbound.max:int=51");

    EncoderPresetLevel level0;
    level0.label = tr("Game Mode (20Mbps, QP≤28)");
    level0.tooltip = tr("VBR + hard QP cap at 28 + short GOP. The strongest anti-blur shield for real-time gaming.");
    level0.description = tr("VBR + QP cap at 28 + short GOP (5 frames). Hard ceiling blocks Sticky QP completely — best for real-time gaming.");
    level0.options = gameMode;
    level0.bitRate = 20000000;

    EncoderPresetLevel level1;
    level1.label = tr("Balanced (8Mbps, QP≤35)");
    level1.tooltip = tr("Standard anti-blur protection with optimal visual quality. Recommended for daily use.");
    level1.description = tr("VBR + QP cap at 35 + standard GOP. Matches default visual quality while preventing QP runaway. Ideal for daily use.");
    level1.options = common;
    level1.bitRate = 8000000;

    EncoderPresetLevel level2;
    level2.label = tr("Power Saver (4Mbps, QP≤35)");
    level2.tooltip = tr("Maximum power saving: low-power ME + single ref frame. Saves most encoder power for non-gaming scenarios.");
    level2.description = tr("VBR + QP cap at 35 + disabled visual optimizations + low-power ME + single ref frame. Maximum power saving for non-intensive scenarios.");
    level2.options = common + QStringLiteral(",") + visualOpt
                     + QStringLiteral(",") + meLowPower
                     + QStringLiteral(",") + extreme;
    level2.bitRate = 4000000;

    preset.levels = { level0, level1, level2 };
    return preset;
}

} // namespace

const QVector<EncoderPreset> &EncoderPresetRegistry::all()
{
    static const QVector<EncoderPreset> presets = { makeMtkPreset() };
    return presets;
}

quint32 EncoderPreset::levelBitRate(int level) const
{
    return (level >= 0 && level < levels.size()) ? levels.at(level).bitRate : 0;
}

int EncoderPreset::levelMaxSizeIndex(int level) const
{
    return (level >= 0 && level < levels.size()) ? levels.at(level).maxSizeIndex : -1;
}

void EncoderPreset::buildParams(int level, QString &outOptions, quint32 &outBitRate) const
{
    if (level >= 0 && level < levels.size()) {
        outOptions = levels.at(level).options;
        outBitRate = levels.at(level).bitRate;
    } else {
        outOptions.clear();
        outBitRate = 0;
    }
}
