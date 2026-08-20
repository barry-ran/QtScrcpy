#include "presetconfigdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

PresetConfigDialog::PresetConfigDialog(const EncoderPreset &preset, QWidget *parent)
    : QDialog(parent)
    , m_preset(preset)
{
    setWindowTitle(preset.displayName + QStringLiteral(" ") + tr("Settings"));
    setMinimumWidth(420);

    auto *mainLayout = new QVBoxLayout(this);

    // Warning label
    auto *warningLabel = new QLabel(this);
    warningLabel->setWordWrap(true);
    warningLabel->setStyleSheet(QStringLiteral("color:#e67e22; font-weight:bold; padding:8px;"));
    warningLabel->setText(preset.warning);
    mainLayout->addWidget(warningLabel);

    // --- Encoder level group ---
    auto *levelGroup = new QGroupBox(tr("Encoding Optimization Level"), this);
    auto *levelLayout = new QGridLayout(levelGroup);

    auto *levelLabel = new QLabel(tr("Profile:"), this);
    m_levelBox = new QComboBox(this);

    for (int i = 0; i < preset.levelCount(); ++i) {
        const EncoderPresetLevel &level = preset.levels.at(i);
        m_levelBox->addItem(level.label);
        m_levelBox->setItemData(i, level.tooltip, Qt::ToolTipRole);
    }
    m_levelBox->setCurrentIndex(m_selectedLevel);

    auto *levelDesc = new QLabel(this);
    levelDesc->setWordWrap(true);
    levelDesc->setStyleSheet(QStringLiteral("color:#888;"));
    auto updateLevelDesc = [this, levelDesc](int idx) {
        if (idx >= 0 && idx < m_preset.levelCount()) {
            levelDesc->setText(m_preset.levels.at(idx).description);
        } else {
            levelDesc->clear();
        }
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

void PresetConfigDialog::setLevel(int level)
{
    if (level >= 0 && level < m_preset.levelCount()) {
        m_selectedLevel = level;
        m_levelBox->setCurrentIndex(level);
    }
}
