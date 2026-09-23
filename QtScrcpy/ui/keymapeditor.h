#ifndef KEYMAPEDITOR_H
#define KEYMAPEDITOR_H

#include <QDialog>
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
#include <QScrollArea>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSlider>

// Helper to convert Qt Key to string and friendly name
QString qtKeyToString(int key);
QString friendlyKeyName(const QString &keyStr);

// Data structure for one keymap node
struct KeyNode {
    enum Type { Click, ClickTwice, ClickMulti, SteerWheel, Drag };

    Type    type        = Click;
    QString comment;

    // Click / Double Click / Multi Click
    QString key         = "Key_Space";
    QPointF pos         = QPointF(0.5, 0.5); // 0.0 - 1.0 ratio
    bool    switchMap   = false;

    // SteerWheel (Joystick WASD)
    QPointF centerPos   = QPointF(0.2, 0.7);
    double  leftOffset  = 0.1;
    double  rightOffset = 0.1;
    double  upOffset    = 0.1;
    double  downOffset  = 0.1;
    QString leftKey     = "Key_A";
    QString rightKey    = "Key_D";
    QString upKey       = "Key_W";
    QString downKey     = "Key_S";

    // Drag
    QPointF startPos    = QPointF(0.3, 0.5);
    QPointF endPos      = QPointF(0.7, 0.5);

    QJsonObject toJson() const;
    static KeyNode fromJson(const QJsonObject &o);
};

// Interactive Key Recording Button
class KeyRecordButton : public QPushButton
{
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

// Canvas widget - draws phone screen, all keymap nodes, handles drag & drop
class KeymapCanvas : public QWidget
{
    Q_OBJECT
public:
    enum Orientation { Landscape, Portrait };

    explicit KeymapCanvas(QWidget *parent = nullptr);

    void setNodes(QList<KeyNode> *nodes);
    void setSelectedIndex(int idx);
    int  selectedIndex() const { return m_selIdx; }

    void setOrientation(Orientation o);
    Orientation orientation() const { return m_orientation; }

    void setMouseMoveMap(bool enabled, QPointF startPos, double speedX, double speedY, const QString &eyeKey, QPointF eyePos);
    bool hasMouseMoveMap() const { return m_hasMouseMove; }
    QPointF mouseMoveStartPos() const { return m_mouseStartPos; }
    QPointF smallEyesPos() const { return m_eyePos; }

signals:
    void nodeSelected(int idx);
    void nodeMoved(int idx, QPointF newRatio);
    void mouseAimMoved(QPointF newRatio);
    void smallEyesMoved(QPointF newRatio);
    void canvasDoubleClicked(QPointF ratio);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    QRect   canvasRect() const;
    QPoint  ratioToPixel(QPointF r) const;
    QPointF pixelToRatio(QPoint p) const;
    int     hitTest(QPoint p) const; // -1: none, 0+: node idx, -2: mouseAim, -3: smallEyes

    QList<KeyNode> *m_nodes        = nullptr;
    int             m_selIdx       = -1; // -1: none, 0+: node, -2: mouseAim, -3: smallEyes
    bool            m_drag         = false;
    QPoint          m_dragOffset;
    Orientation     m_orientation  = Landscape;

    // Mouse Move & Small Eyes
    bool            m_hasMouseMove = false;
    QPointF         m_mouseStartPos= QPointF(0.5, 0.5);
    double          m_speedX       = 3.0;
    double          m_speedY       = 1.5;
    QString         m_eyeKey       = "Key_Alt";
    QPointF         m_eyePos       = QPointF(0.8, 0.3);
};

// Main Dialog - TC Games Style Keymap Editor
class KeymapEditor : public QDialog
{
    Q_OBJECT
public:
    explicit KeymapEditor(const QString &serial, QWidget *parent = nullptr);
    ~KeymapEditor();

private slots:
    void onNodeSelected(int idx);
    void onNodeMoved(int idx, QPointF newRatio);
    void onMouseAimMoved(QPointF newRatio);
    void onSmallEyesMoved(QPointF newRatio);
    void onCanvasDoubleClicked(QPointF ratio);

    void onAddClick();
    void onAddClickTwice();
    void onAddJoystick();
    void onAddMouseAim();
    void onAddSmallEyes();
    void onDeleteSelected();
    void onDuplicateSelected();

    void onApplyProps();
    void onSaveAndApply();
    void onSaveAs();
    void onLoadPreset(int index);
    void onImportFile();
    void onNewLayout();
    void onToggleOrientation();

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void buildUI();
    void applyTheme();
    void refreshPresetList();
    void populateList();
    void loadPropsToUI(int idx);
    void savePropsFromUI(int idx);

    QString defaultKeymapDir() const;
    QString userKeymapDir() const;
    QString currentFilePath() const;
    bool loadJson(const QString &path);
    bool saveJson(const QString &path);
    void applyToDevice();

    // State
    QString          m_serial;
    QList<KeyNode>   m_nodes;
    QString          m_switchKey      = "Key_QuoteLeft";
    bool             m_hasMouseMove   = true;
    QPointF          m_mouseStartPos  = QPointF(0.55, 0.5);
    double           m_mouseSpeedX    = 3.0;
    double           m_mouseSpeedY    = 1.5;
    bool             m_hasSmallEyes   = false;
    QString          m_smallEyesKey   = "Key_Alt";
    QPointF          m_smallEyesPos   = QPointF(0.8, 0.3);
    double           m_smallEyesSpeed = 10.0;
    QString          m_currentFile;
    int              m_selIdx         = -1;

    // UI
    KeymapCanvas    *m_canvas         = nullptr;
    QListWidget     *m_nodeList       = nullptr;
    QComboBox       *m_presetCombo    = nullptr;
    QPushButton     *m_orientBtn      = nullptr;
    QLabel          *m_fileStatusLbl  = nullptr;

    // Properties Panel
    QGroupBox       *m_propsGroup     = nullptr;
    QLineEdit       *m_commentEdit    = nullptr;
    QComboBox       *m_typeCombo      = nullptr;

    // Click Widget
    QWidget         *m_clickWidget    = nullptr;
    KeyRecordButton *m_keyRecordBtn   = nullptr;
    QLineEdit       *m_keyTextEdit    = nullptr;
    QCheckBox       *m_switchChk      = nullptr;

    // Joystick Widget
    QWidget         *m_joyWidget      = nullptr;
    KeyRecordButton *m_upKeyBtn       = nullptr;
    KeyRecordButton *m_downKeyBtn     = nullptr;
    KeyRecordButton *m_leftKeyBtn     = nullptr;
    KeyRecordButton *m_rightKeyBtn    = nullptr;
    QDoubleSpinBox  *m_offsetSpin     = nullptr;

    // Mouse Aim Widget
    QWidget         *m_aimWidget      = nullptr;
    KeyRecordButton *m_switchKeyBtn   = nullptr;
    QDoubleSpinBox  *m_aimSpeedXSpin  = nullptr;
    QDoubleSpinBox  *m_aimSpeedYSpin  = nullptr;

    // Small Eyes Widget
    QWidget         *m_eyeWidget      = nullptr;
    KeyRecordButton *m_eyeKeyBtn      = nullptr;

    QPushButton     *m_deleteBtn      = nullptr;
    QPushButton     *m_duplicateBtn   = nullptr;
    QPushButton     *m_applyPropsBtn  = nullptr;
};

#endif // KEYMAPEDITOR_H