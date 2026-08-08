#ifndef MTKCONFIGDIALOG_H
#define MTKCONFIGDIALOG_H

#include <QDialog>

class QComboBox;

class MtkConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MtkConfigDialog(QWidget *parent = nullptr);

    int selectedLevel() const { return m_selectedLevel; }
    void setLevel(int level);

    static void buildLevelParams(int index, QString &outOptions,
                                 QString &outCodecName, quint32 &outBitRate);
    static quint32 levelBitRate(int level);
    // Returns the recommended maxSizeBox index for the given level,
    // or -1 if the level does not prescribe a specific max size.
    static int levelMaxSizeIndex(int level);

signals:
    void levelChanged(int level);

private:
    QComboBox* m_levelBox;
    int m_selectedLevel = 1;  // 0=Game Mode(20Mbps), 1=Balanced(8Mbps), 2=Power Saver(4Mbps)

    friend class Dialog;
};

#endif // MTKCONFIGDIALOG_H
