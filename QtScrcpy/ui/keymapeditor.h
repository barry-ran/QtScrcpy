#ifndef KEYMAPEDITOR_H
#define KEYMAPEDITOR_H

#include <QWidget>
#include <QList>
#include <QString>
#include <QPointF>
#include <QJsonObject>
#include <QJsonArray>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QScrollArea>
#include <QFrame>

QString qtKeyToString(int key);
QString friendlyKeyName(const QString &keyStr);

struct KeyNode {
    enum Type { Click, ClickTwice, ClickMulti, SteerWheel, Drag };
    Type    type        = Click;
    QString comment;
    QString key         = "Key_Space";
    QPointF pos         = QPointF(0.5, 0.5);
    bool    switchMap   = false;
    QPointF centerPos   = QPointF(0.2, 0.7);
    double  leftOffset  = 0.1;
    double  rightOffset = 0.1;
    double  upOffset    = 0.1;
    double  downOffset  = 0.1;
    QString leftKey     = "Key_A";
    QString rightKey    = "Key_D";
    QString upKey       = "Key_W";
    QString downKey     = "Key_S";
    QPointF startPos    = QPointF(0.3, 0.5);
    QPointF endPos      = QPointF(0.7, 0.5);
    QJsonObject toJson() const;
    static KeyNode fromJson(const QJsonObject &o);
};

class KeyRecordButton : public QPushButton {
    Q_OBJECT
public:
    explicit KeyRecordButton(QWidget *parent = nullptr);
    void setRecordedKey(const QString &keyName);
    QString recordedKey() const { return m_keyName; }
signals:
    void keyChanged(const QString &newKey);
protected:
    void mousePressEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
private:
    bool    m_recording = false;
    QString m_keyName   = "Key_Space";
    void updateButtonText();
};

// Transparent widget rendered ON TOP of the phone video display
class KeymapOverlay : public QWidget {
    Q_OBJECT
public:
    explicit KeymapOverlay(QWidget *parent = nullptr);
    void setNodes(QList<KeyNode> *nodes);
    void setSelectedIndex(int idx);
    int  selectedIndex() const { return m_selIdx; }
    void setMouseMoveMap(bool enabled, QPointF startPos, double speedX, double speedY,
                         const QString &eyeKey, QPointF eyePos);
    bool    hasMouseMove()      const { return m_hasMouseMove; }
    QPointF mouseMoveStartPos() const { return m_mouseStartPos; }
    QPointF smallEyesPos()      const { return m_eyePos; }
signals:
    void nodeSelected(int idx);
    void nodeMoved(int idx, QPointF newRatio);
    void mouseAimMoved(QPointF newRatio);
    void smallEyesMoved(QPointF newRatio);
    void overlayDoubleClicked(QPointF ratio);
    void requestDelete(int idx);
protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
private:
    QPoint  ratioToPixel(QPointF r) const;
    QPointF pixelToRatio(QPoint p)  const;
    int     hitTest(QPoint p)       const;
    QList<KeyNode> *m_nodes         = nullptr;
    int             m_selIdx        = -1;
    bool            m_drag          = false;
    QPoint          m_dragOffset;
    bool            m_hasMouseMove  = false;
    QPointF         m_mouseStartPos = QPointF(0.5, 0.5);
    double          m_speedX        = 3.0;
    double          m_speedY        = 1.5;
    QString         m_eyeKey        = "Key_Alt";
    QPointF         m_eyePos        = QPointF(0.8, 0.3);
};

// Compact dark side panel with add-buttons + properties + save/apply
class KeymapSidePanel : public QFrame {
    Q_OBJECT
public:
    explicit KeymapSidePanel(QWidget *parent = nullptr);
    void loadNode(int idx, QList<KeyNode> *nodes, bool hasMouseMove, QPointF mousePos,
                  double speedX, double speedY, bool hasSmallEyes,
                  const QString &eyeKey, QPointF eyePos);
    void clearProps();
    void saveNodeProps(int idx, QList<KeyNode> *nodes);
    void readMouseAimProps(bool &hasMouseMove, QPointF &mousePos, double &speedX, double &speedY);
    void readSmallEyesProps(bool &hasSmallEyes, QString &eyeKey, QPointF &eyePos);
    QString currentFile() const { return m_currentFile; }
    void setCurrentFile(const QString &f);
signals:
    void addClick();
    void addClickTwice();
    void addJoystick();
    void addMouseAim();
    void addSmallEyes();
    void applyProps();
    void deleteSelected();
    void duplicateSelected();
    void saveAndApply();
    void saveAs();
    void importFile();
    void newLayout();
    void closeOverlay();
private:
    void buildUI();
    QLabel          *m_fileLabel    = nullptr;
    QString          m_currentFile;
    QScrollArea     *m_propsScroll  = nullptr;
    QGroupBox       *m_propsGroup   = nullptr;
    QLineEdit       *m_commentEdit  = nullptr;
    QComboBox       *m_typeCombo    = nullptr;
    QWidget         *m_clickWidget  = nullptr;
    KeyRecordButton *m_keyBtn       = nullptr;
    QCheckBox       *m_switchChk    = nullptr;
    QWidget         *m_joyWidget    = nullptr;
    KeyRecordButton *m_upBtn        = nullptr;
    KeyRecordButton *m_downBtn      = nullptr;
    KeyRecordButton *m_leftBtn      = nullptr;
    KeyRecordButton *m_rightBtn     = nullptr;
    QDoubleSpinBox  *m_offsetSpin   = nullptr;
    QWidget         *m_aimWidget    = nullptr;
    QDoubleSpinBox  *m_speedXSpin   = nullptr;
    QDoubleSpinBox  *m_speedYSpin   = nullptr;
    QWidget         *m_eyeWidget    = nullptr;
    KeyRecordButton *m_eyeKeyBtn    = nullptr;
    QPushButton     *m_applyBtn     = nullptr;
    QPushButton     *m_deleteBtn    = nullptr;
    QPushButton     *m_dupBtn       = nullptr;
};

// Controller: owns overlay + panel, attaches to VideoForm. Replaces old QDialog.
class KeymapEditorController : public QObject {
    Q_OBJECT
public:
    explicit KeymapEditorController(const QString &serial, QWidget *videoContainer,
                                    QWidget *sideParent, QObject *parent = nullptr);
    ~KeymapEditorController();
    void show();
    void hide();
    bool isVisible() const;
protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
private slots:
    void onNodeSelected(int idx);
    void onNodeMoved(int idx, QPointF r);
    void onMouseAimMoved(QPointF r);
    void onSmallEyesMoved(QPointF r);
    void onOverlayDoubleClicked(QPointF r);
    void onRequestDelete(int idx);
    void onAddClick();
    void onAddClickTwice();
    void onAddJoystick();
    void onAddMouseAim();
    void onAddSmallEyes();
    void onApplyProps();
    void onDeleteSelected();
    void onDuplicateSelected();
    void onSaveAndApply();
    void onSaveAs();
    void onImportFile();
    void onNewLayout();
    void onClose();
private:
    void syncOverlay();
    bool loadJson(const QString &path);
    bool saveJson(const QString &path);
    void applyToDevice();
    QString defaultKeymapDir() const;
    QString userKeymapDir()    const;
    QString currentFilePath()  const;
    QString          m_serial;
    QWidget         *m_videoContainer  = nullptr;
    QWidget         *m_sideParent      = nullptr;
    KeymapOverlay   *m_overlay         = nullptr;
    KeymapSidePanel *m_panel           = nullptr;
    QList<KeyNode>   m_nodes;
    int              m_selIdx          = -1;
    bool             m_hasMouseMove    = true;
    QPointF          m_mouseStartPos   = QPointF(0.55, 0.5);
    double           m_mouseSpeedX     = 3.0;
    double           m_mouseSpeedY     = 1.5;
    bool             m_hasSmallEyes    = false;
    QString          m_smallEyesKey    = "Key_Alt";
    QPointF          m_smallEyesPos    = QPointF(0.8, 0.3);
    QString          m_switchKey       = "Key_QuoteLeft";
    QString          m_currentFile;
};

#endif // KEYMAPEDITOR_H