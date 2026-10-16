#ifndef KEYOVERLAYWIDGET_H
#define KEYOVERLAYWIDGET_H

#include <QPointF>
#include <QString>
#include <QVector>
#include <QWidget>

class QEvent;
class QJsonObject;
class QPaintEvent;

class KeyOverlayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KeyOverlayWidget(QWidget *parent = nullptr);

    bool setScript(const QString &script);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    struct Marker
    {
        QPointF position;
        QPointF endPosition;
        QString label;
        bool hasEndPosition = false;
    };

    static bool readPosition(const QJsonObject &object, const QString &key, QPointF &position);
    static QString displayKey(const QString &key);
    void addMarker(const QPointF &position, const QString &key);

    QVector<Marker> m_markers;
};

#endif // KEYOVERLAYWIDGET_H
