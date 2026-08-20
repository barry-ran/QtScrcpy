#ifndef PRESETCONFIGDIALOG_H
#define PRESETCONFIGDIALOG_H

#include <QDialog>

#include "encoderpreset.h"

class QComboBox;

// Generic dialog for configuring a single EncoderPreset. It renders the
// preset's warning and optimization tiers, so any registered preset can be
// edited without dialog-specific code.
class PresetConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PresetConfigDialog(const EncoderPreset &preset, QWidget *parent = nullptr);

    int selectedLevel() const { return m_selectedLevel; }
    void setLevel(int level);
    QString presetId() const { return m_preset.id; }

signals:
    void levelChanged(int level);

private:
    const EncoderPreset &m_preset;
    QComboBox *m_levelBox;
    int m_selectedLevel = 0;
};

#endif // PRESETCONFIGDIALOG_H
