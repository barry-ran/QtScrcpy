#include "mtkconfigdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

MtkConfigDialog::MtkConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Encoder Settings (MediaTek Only)"));
    setMinimumWidth(420);

    auto *mainLayout = new QVBoxLayout(this);

    // Warning label
    auto *warningLabel = new QLabel(this);
    warningLabel->setWordWrap(true);
    warningLabel->setStyleSheet(QStringLiteral("color:#e67e22; font-weight:bold; padding:8px;"));
    warningLabel->setText(tr(
        "This feature is for MediaTek (MTK) devices only.\n"
        "Using on non-MTK devices may cause encoding failure or visual artifacts."));
    mainLayout->addWidget(warningLabel);

    // --- Encoder level group ---
    auto *levelGroup = new QGroupBox(tr("Encoding Optimization Level"), this);
    auto *levelLayout = new QGridLayout(levelGroup);

    auto *levelLabel = new QLabel(tr("Profile:"), this);
    m_levelBox = new QComboBox(this);

    // Level labels and tooltips (3 levels)
    static const char* levelLabels[] = {
        QT_TR_NOOP("Game Mode (20Mbps, QP≤28)"),
        QT_TR_NOOP("Balanced (8Mbps, QP≤35)"),
        QT_TR_NOOP("Power Saver (4Mbps, QP≤35)")
    };
    static const char* levelTooltips[] = {
        QT_TR_NOOP("VBR + hard QP cap at 28 + short GOP. The strongest anti-blur shield for real-time gaming."),
        QT_TR_NOOP("Standard anti-blur protection with optimal visual quality. Recommended for daily use."),
        QT_TR_NOOP("Maximum power saving: low-power ME + single ref frame. Saves most encoder power for non-gaming scenarios.")
    };

    for (int i = 0; i < 3; ++i) {
        m_levelBox->addItem(tr(levelLabels[i]));
        m_levelBox->setItemData(i, tr(levelTooltips[i]), Qt::ToolTipRole);
    }
    m_levelBox->setCurrentIndex(m_selectedLevel);

    auto *levelDesc = new QLabel(this);
    levelDesc->setWordWrap(true);
    levelDesc->setStyleSheet(QStringLiteral("color:#888;"));
    auto updateLevelDesc = [levelDesc](int idx) {
        static const char* descs[] = {
            QT_TR_NOOP("VBR + QP cap at 28 + short GOP (5 frames). Hard ceiling blocks Sticky QP completely — best for real-time gaming."),
            QT_TR_NOOP("VBR + QP cap at 35 + standard GOP. Matches default visual quality while preventing QP runaway. Ideal for daily use."),
            QT_TR_NOOP("VBR + QP cap at 35 + disabled visual optimizations + low-power ME + single ref frame. Maximum power saving for non-intensive scenarios.")
        };
        levelDesc->setText(tr(descs[idx]));
    };
    updateLevelDesc(m_levelBox->currentIndex());
    connect(m_levelBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, updateLevelDesc](int idx) {
        m_selectedLevel = idx;
        updateLevelDesc(idx);
        emit levelChanged(idx);
    });

    levelLayout->addWidget(levelLabel, 0, 0);
    levelLayout->addWidget(m_levelBox, 0, 1);
    levelLayout->addWidget(levelDesc, 1, 0, 1, 2);
    mainLayout->addWidget(levelGroup);

    // --- Close button ---
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
    mainLayout->addWidget(buttonBox);
}

void MtkConfigDialog::setLevel(int level)
{
    if (level >= 0 && level <= 2) {
        m_selectedLevel = level;
        m_levelBox->setCurrentIndex(level);
    }
}

// static
void MtkConfigDialog::buildLevelParams(int index, QString &outOptions, QString &outCodecName, quint32 &outBitRate)
{
    // All 3 levels use VBR (bitrate-mode=1) — CBR is not supported on
    // c2.mtk.avc.encoder (confirmed on MT6895, Android 13).
    //
    // Level 0 "Game Mode": tight QP ceiling (28) + short GOP (5)
    //   The hard QP cap physically prevents Sticky QP from ever degrading
    //   the frame. Even if the encoder's rate control tries to raise QP,
    //   it cannot exceed 28 — the frame stays sharp.
    static const QString gameMode = QStringLiteral(
        "profile=1,"
        "max-bframes=0,"
        "i-frame-interval=5,"
        "priority=0,"
        "bitrate-mode=1,"
        "video-qp-min=15,"
        "video-qp-max=28");

    // Level 1 "Balanced" / Level 2 "Power Saver": standard QP cap (35)
    static const QString common = QStringLiteral(
        "profile=1,"
        "max-bframes=0,"
        "i-frame-interval=10,"
        "priority=0,"
        "bitrate-mode=1,"
        "video-qp-max=35,"
        "vendor.mtk.ext.venc.i.frame.control.size.max-i-ratio:int=30,"
        "vendor.mtk.ext.venc.mbrc.tracking-speed:int=0");

    // Level 2 extras: disable visual optimizations, low-power ME, single ref frame
    static const QString visualOpt = QStringLiteral(
        "vendor.mtk.ext.venc.visual.rd:int=0,"
        "vendor.mtk.ext.venc.visual.quant:int=0");

    static const QString meLowPower = QStringLiteral(
        "vendor.mtk.ext.venc.highquality.feature-on:int=1,"
        "vendor.mtk.ext.venc.highquality.mode:int=0");

    static const QString extreme = QStringLiteral(
        "vendor.mtk.ext.venc.ref.frame.num:int=1,"
        "vendor.mtk.venc.dynamic.qpbound.min:int=28,"
        "vendor.mtk.venc.dynamic.qpbound.max:int=51");

    outCodecName = QStringLiteral("c2.mtk.avc.encoder");

    switch (index) {
    case 0: // Game Mode  20Mbps
        outOptions = gameMode;
        outBitRate = 20000000;
        break;
    case 1: // Balanced      8Mbps
        outOptions = common;
        outBitRate = 8000000;
        break;
    case 2: // Power Saver   4Mbps
        outOptions = common + QStringLiteral(",") + visualOpt
                     + QStringLiteral(",") + meLowPower
                     + QStringLiteral(",") + extreme;
        outBitRate = 4000000;
        break;
    default:
        outOptions.clear();
        outCodecName.clear();
        outBitRate = 0;
        break;
    }
}

// static
quint32 MtkConfigDialog::levelBitRate(int level)
{
    switch (level) {
    case 0: return 20000000;  // Game Mode
    case 1: return 8000000;   // Balanced
    case 2: return 4000000;   // Power Saver
    default: return 0;
    }
}

// static
int MtkConfigDialog::levelMaxSizeIndex(int level)
{
    Q_UNUSED(level);
    // No level prescribes a max size — the QP hard cap works
    // independently of resolution. Let the user decide.
    return -1;
}
