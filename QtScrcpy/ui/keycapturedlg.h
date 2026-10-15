#ifndef KEYCAPTUREDLG_H
#define KEYCAPTUREDLG_H

#include <QDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>

/**
 * @brief A modal dialog that waits for a single key/mouse-button press
 *        and returns its Qt name as a string.
 *
 * Usage:
 *   KeyCaptureDlg dlg(this);
 *   if (dlg.exec() == QDialog::Accepted)
 *       QString key = dlg.capturedKey();   // e.g. "Key_F", "LeftButton"
 */
class KeyCaptureDlg : public QDialog
{
    Q_OBJECT
public:
    explicit KeyCaptureDlg(QWidget *parent = nullptr);

    /** Returns the captured key string (empty if cancelled). */
    const QString &capturedKey() const { return m_captured; }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setCaptured(const QString &key);

    QLabel      *m_hint    = nullptr;
    QLabel      *m_result  = nullptr;
    QPushButton *m_accept  = nullptr;
    QPushButton *m_cancel  = nullptr;
    QString      m_captured;
};

#endif // KEYCAPTUREDLG_H
