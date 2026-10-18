#ifndef KEYOVERLAYWIDGET_H
#define KEYOVERLAYWIDGET_H

#include <QJsonObject>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QWidget>

class QEvent;
class QLabel;
class QMouseEvent;
class QPaintEvent;
class QPushButton;

class KeyOverlayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KeyOverlayWidget(QWidget *parent = nullptr);

    bool setScript(const QString &script);
    bool setEditMode(bool enabled);
    void cancelEditing();
    void requestSave();
    bool isEditMode() const { return m_editMode; }

signals:
    void saveRequested(const QString &script);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    enum class PositionTarget
    {
        ReadOnly,
        NodeField,
        MultiClickField,
        SteeringDirection,
        MouseMoveStart,
        SmallEyesPosition
    };

    struct Marker
    {
        QPointF position;
        QPointF endPosition;
        QString key;
        QString label;
        QString keyField = QStringLiteral("key");
        QString positionField;
        QString endPositionField;
        QString offsetField;
        PositionTarget target = PositionTarget::ReadOnly;
        int nodeIndex = -1;
        int childIndex = -1;
        bool hasEndPosition = false;
    };

    static bool readPosition(const QJsonObject &object, const QString &key, QPointF &position);
    static QString displayKey(const QString &key);
    static QString parseKeyName(const QString &keyText);
    static QJsonObject toJsonPosition(const QPointF &position);
    void addMarker(const QPointF &position, const QString &key);
    void addNodeMarker(const QPointF &position, const QString &key, int nodeIndex,
                       const QString &positionField, PositionTarget target = PositionTarget::NodeField,
                       int childIndex = -1, const QString &keyField = QStringLiteral("key"));
    void rebuildMarkers();
    void updateEditToolbarGeometry();
    int markerAt(const QPointF &position, bool *atEnd = nullptr) const;
    int editableMarkerForKey(const QString &key) const;
    bool keyIsUsed(const QString &key, int exceptNodeIndex = -1) const;
    bool setMarkerPosition(int markerIndex, bool endPoint, const QPointF &position);
    bool setMarkerKey(int markerIndex, const QString &key);
    void addMappingAt(const QPointF &position);
    void editAppearanceDialog();
    void loadAppearanceSettings();
    void editMarker(int markerIndex, const QPoint &globalPosition);
    bool deleteMarker(int markerIndex);
    QString serializedScript() const;

    QVector<Marker> m_markers;
    QJsonObject m_root;
    QString m_originalScript;
    QWidget *m_editToolbar = nullptr;
    QLabel *m_editHint = nullptr;
    QPushButton *m_styleButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    qreal m_keySize = 1.0;
    qreal m_keyOpacity = 1.0;
    int m_draggingMarker = -1;
    int m_selectedMarkerIndex = -1;
    bool m_draggingEndPoint = false;
    bool m_placingPendingKey = false;
    bool m_editMode = false;
    bool m_validScript = false;
    QString m_pendingKey;
};

#endif // KEYOVERLAYWIDGET_H
