#ifndef KEYMAPOVERLAY_H
#define KEYMAPOVERLAY_H

#include <QWidget>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QPair>

class KeymapNodeWidget;
class DragNodeWidget;
class MultiClickNodeWidget;
class QResizeEvent;
class QLabel;
class QPushButton;

/**
 * @brief Transparent overlay that displays keymap nodes over the video widget.
 *
 * In edit mode, a side palette adds controls to the video surface. Nodes can
 * be dragged, reassigned, or removed before the map is saved and applied.
 *
 * When edit mode is turned off the overlay becomes fully transparent to
 * mouse events so it doesn't interfere with normal game input.
 */
class KeymapOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit KeymapOverlay(QWidget *parent = nullptr);
    ~KeymapOverlay();

    void loadKeymap(const QString &jsonFilePath);
    bool saveKeymap(const QString &jsonFilePath);

    void setEditMode(bool edit);
    bool isEditMode() const { return m_editMode; }

signals:
    void saveRequested();
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void addNodeAt(KeymapNodeWidget *node, const QPoint &pos);
    void registerNode(KeymapNodeWidget *node, const QJsonValue &preservedValue = QJsonValue());
    void beginAddAction(const QString &action);
    void finishMultiTap();
    void clearKeymap();
    void updatePanelGeometry();

    bool m_editMode = false;
    QList<KeymapNodeWidget *> m_nodes;
    QList<QPair<KeymapNodeWidget *, QJsonValue>> m_nodeEntries;
    QJsonObject m_document;
    bool m_loadedMouseMoveMap = false;
    bool m_loadFailed = false;
    QWidget *m_toolPanel = nullptr;
    QWidget *m_toolPanelBody = nullptr;
    QLabel *m_panelStatus = nullptr;
    QString m_pendingAction;
    DragNodeWidget *m_pendingDragNode = nullptr;
    MultiClickNodeWidget *m_pendingMultiClickNode = nullptr;
    QPushButton *m_finishMultiTapButton = nullptr;
};

#endif // KEYMAPOVERLAY_H
