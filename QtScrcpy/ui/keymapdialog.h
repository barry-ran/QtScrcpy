#ifndef KEYMAPDIALOG_H
#define KEYMAPDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QTabWidget>

/**
 * @brief نافذة إدارة الـ Keymap داخل البرنامج مباشرةً
 *
 * تتيح للمستخدم:
 * - عرض قائمة بجميع ملفات الـ Keymap (JSON)
 * - تفعيل keymap مباشرةً على الجهاز المتصل
 * - عرض تفاصيل كل مفتاح في جدول واضح
 * - تعديل ملف JSON من خلال محرر نصي مدمج
 * - إنشاء keymap جديد وحفظه
 * - حذف ملفات موجودة
 */
class KeymapDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeymapDialog(const QString &serial, QWidget *parent = nullptr);
    ~KeymapDialog();

private slots:
    void onKeymapSelected(QListWidgetItem *item);
    void onApplyClicked();
    void onNewClicked();
    void onDeleteClicked();
    void onSaveEditorClicked();

private:
    void setupUI();
    void refreshFileList();
    void loadKeymapDetails(const QString &filePath);
    void loadKeymapToEditor(const QString &filePath);
    bool saveKeymapFromEditor();
    void applyKeymap(const QString &filePath);
    QString resolveKeymapPath(const QString &fileName) const;
    QString getUserKeymapPath() const;
    QString getDefaultKeymapPath() const;
    QString keyTypeToString(const QString &type) const;

    // UI widgets
    QListWidget  *m_fileList       = nullptr;
    QTableWidget *m_detailsTable   = nullptr;
    QLabel       *m_switchKeyLabel = nullptr;
    QLabel       *m_filePathLabel  = nullptr;
    QTextEdit    *m_jsonEditor     = nullptr;
    QTabWidget   *m_tabWidget      = nullptr;
    QPushButton  *m_applyBtn       = nullptr;
    QPushButton  *m_newBtn         = nullptr;
    QPushButton  *m_deleteBtn      = nullptr;
    QPushButton  *m_saveEditorBtn  = nullptr;
    QLabel       *m_statusLabel    = nullptr;

    // State
    QString  m_serial;
    QString  m_currentFile;
};

#endif // KEYMAPDIALOG_H
