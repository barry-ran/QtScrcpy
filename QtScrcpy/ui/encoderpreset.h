#ifndef ENCODERPRESET_H
#define ENCODERPRESET_H

#include <QString>
#include <QVector>

// One optimization tier inside a preset (e.g. MTK "Game Mode" / "Balanced" / "Power Saver").
struct EncoderPresetLevel
{
    QString label;        // combo item text
    QString tooltip;      // combo item tooltip
    QString description;  // longer explanation shown below the combo
    QString options;      // full codec-options string for this tier
    quint32 bitRate = 0;  // 0 = leave the user bitrate untouched
    int maxSizeIndex = -1; // -1 = this tier does not prescribe a max size
};

// A vendor/scenario-specific encoder preset. A preset owns a fixed codec
// (codecName) plus one or more optimization tiers. Presets are plain data so
// adding a new one is just constructing an instance and registering it.
struct EncoderPreset
{
    QString id;            // stable identifier, e.g. "mtk"
    QString displayName;   // shown in the codec-mode combo box
    QString codecName;     // empty = don't override the device codec
    QString warning;       // caution text shown at the top of the dialog
    QVector<EncoderPresetLevel> levels;

    int levelCount() const { return levels.size(); }
    quint32 levelBitRate(int level) const;
    int levelMaxSizeIndex(int level) const;
    void buildParams(int level, QString &outOptions, quint32 &outBitRate) const;
};

class EncoderPresetRegistry
{
public:
    static const QVector<EncoderPreset> &all();
};

#endif // ENCODERPRESET_H
