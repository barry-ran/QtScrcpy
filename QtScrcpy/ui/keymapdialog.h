#ifndef KEYMAPDIALOG_H
#define KEYMAPDIALOG_H

#include <QDialog>

class KeymapDialog : public QDialog
{
    Q_OBJECT
public:
    explicit KeymapDialog(QWidget *parent = nullptr);
    ~KeymapDialog() override;

    void setSerial(const QString &serial);

private:
    QString m_serial;
};

#endif // KEYMAPDIALOG_H
