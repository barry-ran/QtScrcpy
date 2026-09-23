#include "keymapeditor.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QStandardPaths>
#include <QFormLayout>
#include <QMenu>
#include <QMetaEnum>
#include <QInputDialog>

#include "../QtScrcpyCore/include/QtScrcpyCore.h"

// == Helper Functions ============================================

QString qtKeyToString(int key) {
    if (key == Qt::Key_Control) return "Key_Control";
    if (key == Qt::Key_Shift) return "Key_Shift";
    if (key == Qt::Key_Alt) return "Key_Alt";
    if (key == Qt::Key_Space) return "Key_Space";
    if (key == Qt::Key_Tab) return "Key_Tab";
    if (key == Qt::Key_Return || key == Qt::Key_Enter) return "Key_Return";
    if (key == Qt::Key_Escape) return "Key_Escape";
    if (key == Qt::Key_QuoteLeft || key == Qt::Key_AsciiTilde) return "Key_QuoteLeft";
    if (key == Qt::Key_CapsLock) return "Key_CapsLock";
    if (key == Qt::Key_Backspace) return "Key_Backspace";

    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return QString("Key_%1").arg(QChar(key));
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return QString("Key_%1").arg(QChar(key));
    }
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12) {
        return QString("Key_F%1").arg(key - Qt::Key_F1 + 1);
    }

    QMetaEnum metaEnum = QMetaEnum::fromType<Qt::Key>();
    const char *keyName = metaEnum.valueToKey(key);
    if (keyName) {
        return QString(keyName);
    }
    return QString("Key_%1").arg(key);
}

QString friendlyKeyName(const QString &keyStr) {
    if (keyStr.isEmpty()) return "None";
    if (keyStr.startsWith("Key_")) {
        QString sub = keyStr.mid(4);
        if (sub == "QuoteLeft") return "~ (Tilde)";
        if (sub == "Control") return "Ctrl";
        if (sub == "Return") return "Enter";
        if (sub == "Space") return "SPACE";
        return sub;
    }
    return keyStr;
}

// == KeyNode Serialization =======================================

QJsonObject KeyNode::toJson() const {
    QJsonObject o;
    o["comment"] = comment;

    if (type == SteerWheel) {
        o["type"] = "KMT_STEER_WHEEL";
        QJsonObject cp;
        cp["x"] = centerPos.x();
        cp["y"] = centerPos.y();
        o["centerPos"]   = cp;
        o["leftOffset"]  = leftOffset;
        o["rightOffset"] = rightOffset;
        o["upOffset"]    = upOffset;
        o["downOffset"]  = downOffset;
        o["leftKey"]     = leftKey;
        o["rightKey"]    = rightKey;
        o["upKey"]       = upKey;
        o["downKey"]     = downKey;
    } else if (type == ClickTwice) {
        o["type"] = "KMT_CLICK_TWICE";
        o["key"]  = key;
        QJsonObject p;
        p["x"] = pos.x();
        p["y"] = pos.y();
        o["pos"] = p;
    } else if (type == ClickMulti) {
        o["type"] = "KMT_CLICK_MULTI";
        o["key"]  = key;
        QJsonObject p;
        p["x"] = pos.x();
        p["y"] = pos.y();
        o["pos"] = p;
    } else if (type == Drag) {
        o["type"] = "KMT_DRAG";
        QJsonObject sp; sp["x"] = startPos.x(); sp["y"] = startPos.y();
        QJsonObject ep; ep["x"] = endPos.x();   ep["y"] = endPos.y();
        o["startPos"] = sp;
        o["endPos"]   = ep;
    } else {
        o["type"] = "KMT_CLICK";
        o["key"]  = key;
        QJsonObject p;
        p["x"] = pos.x();
        p["y"] = pos.y();
        o["pos"]       = p;
        o["switchMap"] = switchMap;
    }
    return o;
}

KeyNode KeyNode::fromJson(const QJsonObject &o) {
    KeyNode n;
    n.comment = o["comment"].toString();
    QString typeStr = o["type"].toString();

    if (typeStr == "KMT_STEER_WHEEL") {
        n.type        = SteerWheel;
        QJsonObject cp = o["centerPos"].toObject();
        n.centerPos   = QPointF(cp["x"].toDouble(0.2), cp["y"].toDouble(0.7));
        n.leftOffset  = o["leftOffset"].toDouble(0.1);
        n.rightOffset = o["rightOffset"].toDouble(0.1);
        n.upOffset    = o["upOffset"].toDouble(0.1);
        n.downOffset  = o["downOffset"].toDouble(0.1);
        n.leftKey     = o["leftKey"].toString("Key_A");
        n.rightKey    = o["rightKey"].toString("Key_D");
        n.upKey       = o["upKey"].toString("Key_W");
        n.downKey     = o["downKey"].toString("Key_S");
    } else if (typeStr == "KMT_CLICK_TWICE") {
        n.type        = ClickTwice;
        n.key         = o["key"].toString("Key_Q");
        QJsonObject p = o["pos"].toObject();
        n.pos         = QPointF(p["x"].toDouble(0.5), p["y"].toDouble(0.5));
    } else if (typeStr == "KMT_CLICK_MULTI") {
        n.type        = ClickMulti;
        n.key         = o["key"].toString("Key_F");
        QJsonObject p = o["pos"].toObject();
        n.pos         = QPointF(p["x"].toDouble(0.5), p["y"].toDouble(0.5));
    } else if (typeStr == "KMT_DRAG") {
        n.type        = Drag;
        QJsonObject sp = o["startPos"].toObject();
        QJsonObject ep = o["endPos"].toObject();
        n.startPos    = QPointF(sp["x"].toDouble(0.3), sp["y"].toDouble(0.5));
        n.endPos      = QPointF(ep["x"].toDouble(0.7), ep["y"].toDouble(0.5));
    } else {
        n.type        = Click;
        n.key         = o["key"].toString("Key_Space");
        QJsonObject p = o["pos"].toObject();
        n.pos         = QPointF(p["x"].toDouble(0.5), p["y"].toDouble(0.5));
        n.switchMap   = o["switchMap"].toBool(false);
    }
    return n;
}

// == KeyRecordButton =============================================

KeyRecordButton::KeyRecordButton(QWidget *parent) : QPushButton(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    updateButtonText();
}

void KeyRecordButton::setRecordedKey(const QString &keyName) {
    m_keyName = keyName;
    m_recording = false;
    updateButtonText();
}

void KeyRecordButton::updateButtonText() {
    if (m_recording) {
        setText("Press key...");
        setStyleSheet("background: #501525; border: 2px solid #ff4757; color: #ff6b81; font-weight: bold; border-radius: 4px; padding: 6px;");
    } else {
        setText(QString("%1").arg(friendlyKeyName(m_keyName)));
        setStyleSheet("background: #252538; border: 1px solid #3d3d5c; color: #00d2ff; font-weight: bold; border-radius: 4px; padding: 6px;");
    }
}

void KeyRecordButton::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_recording = true;
        updateButtonText();
        grabKeyboard();
    }
    QPushButton::mousePressEvent(e);
}

void KeyRecordButton::keyPressEvent(QKeyEvent *e) {
    if (m_recording) {
        int k = e->key();
        releaseKeyboard();
        m_recording = false;
        if (k != Qt::Key_Escape) {
            m_keyName = qtKeyToString(k);
            emit keyChanged(m_keyName);
        }
        updateButtonText();
        e->accept();
        return;
    }
    QPushButton::keyPressEvent(e);
}

void KeyRecordButton::focusOutEvent(QFocusEvent *e) {
    if (m_recording) {
        releaseKeyboard();
        m_recording = false;
        updateButtonText();
    }
    QPushButton::focusOutEvent(e);
}
// == KeymapCanvas ===============================================

KeymapCanvas::KeymapCanvas(QWidget *parent) : QWidget(parent) {
    setMinimumSize(460, 300);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background: #10101a; border-radius: 10px;");
}

void KeymapCanvas::setNodes(QList<KeyNode> *n) {
    m_nodes = n;
    update();
}

void KeymapCanvas::setSelectedIndex(int idx) {
    m_selIdx = idx;
    update();
}

void KeymapCanvas::setOrientation(Orientation o) {
    m_orientation = o;
    update();
}

void KeymapCanvas::setMouseMoveMap(bool enabled, QPointF startPos, double speedX, double speedY, const QString &eyeKey, QPointF eyePos) {
    m_hasMouseMove = enabled;
    m_mouseStartPos = startPos;
    m_speedX = speedX;
    m_speedY = speedY;
    m_eyeKey = eyeKey;
    m_eyePos = eyePos;
    update();
}

QRect KeymapCanvas::canvasRect() const {
    int margin = 16;
    int maxW = width() - margin * 2;
    int maxH = height() - margin * 2;

    int targetW, targetH;
    if (m_orientation == Landscape) {
        if (maxW * 9 > maxH * 16) {
            targetH = maxH;
            targetW = targetH * 16 / 9;
        } else {
            targetW = maxW;
            targetH = targetW * 9 / 16;
        }
    } else {
        if (maxW * 16 > maxH * 9) {
            targetH = maxH;
            targetW = targetH * 9 / 16;
        } else {
            targetW = maxW;
            targetH = targetW * 16 / 9;
        }
    }

    return QRect((width() - targetW) / 2, (height() - targetH) / 2, targetW, targetH);
}

QPoint KeymapCanvas::ratioToPixel(QPointF r) const {
    QRect cr = canvasRect();
    return QPoint(cr.x() + static_cast<int>(r.x() * cr.width()),
                  cr.y() + static_cast<int>(r.y() * cr.height()));
}

QPointF KeymapCanvas::pixelToRatio(QPoint p) const {
    QRect cr = canvasRect();
    double rx = qBound(0.0, static_cast<double>(p.x() - cr.x()) / cr.width(), 1.0);
    double ry = qBound(0.0, static_cast<double>(p.y() - cr.y()) / cr.height(), 1.0);
    return QPointF(rx, ry);
}

int KeymapCanvas::hitTest(QPoint p) const {
    QRect cr = canvasRect();
    int btnRadius = qMax(16, cr.width() / 24);

    if (m_hasMouseMove) {
        QPoint aimP = ratioToPixel(m_mouseStartPos);
        if ((p - aimP).manhattanLength() <= btnRadius + 8) return -2;

        QPoint eyeP = ratioToPixel(m_eyePos);
        if ((p - eyeP).manhattanLength() <= btnRadius + 8) return -3;
    }

    if (!m_nodes) return -1;

    for (int i = m_nodes->size() - 1; i >= 0; --i) {
        const KeyNode &n = m_nodes->at(i);
        if (n.type == KeyNode::SteerWheel) {
            QPoint c = ratioToPixel(n.centerPos);
            int joyR = qMax(30, cr.width() / 10);
            if ((p - c).manhattanLength() <= joyR + 8) return i;
        } else if (n.type == KeyNode::Drag) {
            QPoint sp = ratioToPixel(n.startPos);
            QPoint ep = ratioToPixel(n.endPos);
            if ((p - sp).manhattanLength() <= btnRadius + 6 || (p - ep).manhattanLength() <= btnRadius + 6) return i;
        } else {
            QPoint c = ratioToPixel(n.pos);
            if ((p - c).manhattanLength() <= btnRadius + 8) return i;
        }
    }
    return -1;
}

void KeymapCanvas::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    QRect cr = canvasRect();

    // Draw Phone Shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 100));
    p.drawRoundedRect(cr.adjusted(4, 6, 6, 8), 16, 16);

    // Draw Phone Screen Frame (TC Gaming dark slate aesthetic)
    QLinearGradient bgGrad(cr.topLeft(), cr.bottomRight());
    bgGrad.setColorAt(0.0, QColor(24, 24, 40));
    bgGrad.setColorAt(1.0, QColor(14, 14, 26));
    p.setBrush(bgGrad);
    p.setPen(QPen(QColor(60, 60, 95), 2));
    p.drawRoundedRect(cr, 14, 14);

    // Draw Subtle Grid Lines
    p.setPen(QPen(QColor(255, 255, 255, 12), 1, Qt::DotLine));
    int gridCols = (m_orientation == Landscape) ? 16 : 9;
    int gridRows = (m_orientation == Landscape) ? 9 : 16;
    for (int i = 1; i < gridCols; ++i) {
        int x = cr.x() + cr.width() * i / gridCols;
        p.drawLine(x, cr.y() + 4, x, cr.bottom() - 4);
    }
    for (int j = 1; j < gridRows; ++j) {
        int y = cr.y() + cr.height() * j / gridRows;
        p.drawLine(cr.x() + 4, y, cr.right() - 4, y);
    }

    // Screen Center cross mark
    p.setPen(QPen(QColor(0, 210, 255, 30), 1));
    p.drawLine(cr.center().x() - 15, cr.center().y(), cr.center().x() + 15, cr.center().y());
    p.drawLine(cr.center().x(), cr.center().y() - 15, cr.center().x(), cr.center().y() + 15);

    int btnR = qMax(16, cr.width() / 24);

    // 1. Draw Mouse Move Aim Target (-2) if enabled
    if (m_hasMouseMove) {
        bool sel = (m_selIdx == -2);
        QPoint ap = ratioToPixel(m_mouseStartPos);

        p.setPen(QPen(sel ? QColor(0, 255, 200) : QColor(0, 210, 255, 180), sel ? 3 : 2));
        p.setBrush(sel ? QColor(0, 255, 200, 40) : QColor(0, 210, 255, 20));
        p.drawEllipse(ap, btnR + 6, btnR + 6);
        p.drawLine(ap.x() - btnR - 10, ap.y(), ap.x() + btnR + 10, ap.y());
        p.drawLine(ap.x(), ap.y() - btnR - 10, ap.x(), ap.y() + btnR + 10);

        p.setPen(QColor(0, 210, 255));
        QFont f = p.font(); f.setBold(true); f.setPixelSize(10); p.setFont(f);
        p.drawText(QRect(ap.x() - 40, ap.y() + btnR + 8, 80, 20), Qt::AlignCenter, "Aim Look");

        // Small Eyes (Free Look) (-3)
        bool eyeSel = (m_selIdx == -3);
        QPoint ep = ratioToPixel(m_eyePos);
        p.setPen(QPen(eyeSel ? QColor(255, 200, 0) : QColor(255, 180, 50, 180), eyeSel ? 3 : 2));
        p.setBrush(eyeSel ? QColor(255, 200, 0, 60) : QColor(255, 180, 50, 30));
        p.drawEllipse(ep, btnR, btnR);
        p.setPen(Qt::white);
        p.drawText(QRect(ep.x() - btnR, ep.y() - btnR, btnR * 2, btnR * 2), Qt::AlignCenter, "Eye");
        p.setPen(QColor(255, 200, 50));
        p.drawText(QRect(ep.x() - 40, ep.y() + btnR + 4, 80, 20), Qt::AlignCenter, friendlyKeyName(m_eyeKey));
    }

    // 2. Draw Keymap Nodes
    if (m_nodes) {
        for (int i = 0; i < m_nodes->size(); ++i) {
            const KeyNode &n = m_nodes->at(i);
            bool sel = (i == m_selIdx);

            if (n.type == KeyNode::SteerWheel) {
                QPoint c = ratioToPixel(n.centerPos);
                int joyR = qMax(36, cr.width() / 10);

                p.setBrush(sel ? QColor(0, 180, 255, 60) : QColor(40, 100, 200, 40));
                p.setPen(QPen(sel ? QColor(0, 220, 255) : QColor(60, 140, 240, 180), sel ? 3 : 2));
                p.drawEllipse(c, joyR, joyR);

                p.setBrush(sel ? QColor(0, 220, 255, 150) : QColor(255, 255, 255, 80));
                p.setPen(Qt::NoPen);
                p.drawEllipse(c, joyR / 3, joyR / 3);

                p.setPen(Qt::white);
                QFont f = p.font(); f.setBold(true); f.setPixelSize(11); p.setFont(f);
                p.drawText(QRect(c.x() - 20, c.y() - joyR - 2, 40, 16), Qt::AlignCenter, friendlyKeyName(n.upKey));
                p.drawText(QRect(c.x() - 20, c.y() + joyR - 14, 40, 16), Qt::AlignCenter, friendlyKeyName(n.downKey));
                p.drawText(QRect(c.x() - joyR - 2, c.y() - 8, 20, 16), Qt::AlignCenter, friendlyKeyName(n.leftKey));
                p.drawText(QRect(c.x() + joyR - 18, c.y() - 8, 20, 16), Qt::AlignCenter, friendlyKeyName(n.rightKey));

                p.setPen(QColor(150, 200, 255));
                f.setPixelSize(10); p.setFont(f);
                p.drawText(QRect(c.x() - 40, c.y() + joyR + 6, 80, 16), Qt::AlignCenter, n.comment.isEmpty() ? "WASD" : n.comment);

            } else if (n.type == KeyNode::Drag) {
                QPoint sp = ratioToPixel(n.startPos);
                QPoint ep = ratioToPixel(n.endPos);

                p.setPen(QPen(sel ? QColor(255, 100, 200) : QColor(200, 80, 180, 180), sel ? 3 : 2, Qt::DashLine));
                p.drawLine(sp, ep);

                p.setBrush(QColor(255, 100, 200, 150));
                p.drawEllipse(sp, 8, 8);
                p.drawEllipse(ep, 8, 8);

            } else {
                QPoint c = ratioToPixel(n.pos);

                QColor mainCol = (n.type == KeyNode::ClickTwice) ? QColor(255, 120, 0) :
                                 (n.type == KeyNode::ClickMulti) ? QColor(180, 80, 255) : QColor(0, 210, 255);

                p.setBrush(sel ? QColor(mainCol.red(), mainCol.green(), mainCol.blue(), 180) :
                                 QColor(25, 30, 45, 210));
                p.setPen(QPen(sel ? Qt::white : mainCol, sel ? 3 : 2));
                p.drawEllipse(c, btnR, btnR);

                p.setPen(Qt::white);
                QFont f = p.font(); f.setBold(true);
                QString keyText = friendlyKeyName(n.key);
                f.setPixelSize(keyText.length() > 3 ? qMax(8, btnR / 2 - 2) : qMax(10, btnR / 2));
                p.setFont(f);
                p.drawText(QRect(c.x() - btnR, c.y() - btnR, btnR * 2, btnR * 2), Qt::AlignCenter, keyText);

                if (n.type == KeyNode::ClickTwice) {
                    p.setPen(QColor(255, 200, 0));
                    f.setPixelSize(9); p.setFont(f);
                    p.drawText(QRect(c.x() + btnR - 10, c.y() - btnR - 4, 18, 14), Qt::AlignCenter, "x2");
                }

                if (!n.comment.isEmpty()) {
                    p.setPen(QColor(200, 210, 230));
                    f.setBold(false); f.setPixelSize(10); p.setFont(f);
                    p.drawText(QRect(c.x() - 50, c.y() + btnR + 2, 100, 16), Qt::AlignCenter, n.comment);
                }
            }

            if (sel) {
                QPoint c = (n.type == KeyNode::SteerWheel) ? ratioToPixel(n.centerPos) : ratioToPixel(n.pos);
                int glowR = (n.type == KeyNode::SteerWheel) ? qMax(36, cr.width() / 10) + 6 : btnR + 6;
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(QColor(255, 220, 0, 200), 2, Qt::DashLine));
                p.drawEllipse(c, glowR, glowR);
            }
        }
    }

    if ((!m_nodes || m_nodes->isEmpty()) && !m_hasMouseMove) {
        p.setPen(QColor(255, 255, 255, 90));
        QFont f = p.font(); f.setPixelSize(14); p.setFont(f);
        p.drawText(cr, Qt::AlignCenter, "Double-click anywhere to add a button\nor use the Sidebar tools on the right");
    }
}

void KeymapCanvas::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        int idx = hitTest(e->pos());
        m_selIdx = idx;
        emit nodeSelected(idx);
        update();

        if (idx >= 0 && m_nodes && idx < m_nodes->size()) {
            m_drag = true;
            const KeyNode &n = m_nodes->at(idx);
            QPoint c = (n.type == KeyNode::SteerWheel) ? ratioToPixel(n.centerPos) : ratioToPixel(n.pos);
            m_dragOffset = e->pos() - c;
        } else if (idx == -2) {
            m_drag = true;
            m_dragOffset = e->pos() - ratioToPixel(m_mouseStartPos);
        } else if (idx == -3) {
            m_drag = true;
            m_dragOffset = e->pos() - ratioToPixel(m_eyePos);
        }
    }
    QWidget::mousePressEvent(e);
}

void KeymapCanvas::mouseMoveEvent(QMouseEvent *e) {
    if (m_drag) {
        QPointF newRatio = pixelToRatio(e->pos() - m_dragOffset);
        if (m_selIdx >= 0 && m_nodes && m_selIdx < m_nodes->size()) {
            KeyNode &n = (*m_nodes)[m_selIdx];
            if (n.type == KeyNode::SteerWheel) {
                n.centerPos = newRatio;
            } else {
                n.pos = newRatio;
            }
            emit nodeMoved(m_selIdx, newRatio);
            update();
        } else if (m_selIdx == -2) {
            m_mouseStartPos = newRatio;
            emit mouseAimMoved(newRatio);
            update();
        } else if (m_selIdx == -3) {
            m_eyePos = newRatio;
            emit smallEyesMoved(newRatio);
            update();
        }
    }

    int hit = hitTest(e->pos());
    setCursor(hit != -1 ? Qt::SizeAllCursor : Qt::ArrowCursor);
    QWidget::mouseMoveEvent(e);
}

void KeymapCanvas::mouseReleaseEvent(QMouseEvent *e) {
    m_drag = false;
    QWidget::mouseReleaseEvent(e);
}

void KeymapCanvas::mouseDoubleClickEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        QPointF ratio = pixelToRatio(e->pos());
        emit canvasDoubleClicked(ratio);
    }
    QWidget::mouseDoubleClickEvent(e);
}

void KeymapCanvas::contextMenuEvent(QContextMenuEvent *e) {
    QMenu menu(this);
    menu.setStyleSheet("QMenu { background: #1e1e30; color: #fff; border: 1px solid #3d3d5c; } QMenu::item:selected { background: #00d2ff; color: #000; }");

    QAction *actAddClick = menu.addAction("+ Add Click Button");
    QAction *actAddJoy   = menu.addAction("+ Add WASD Joystick");
    QAction *actAddAim   = menu.addAction("+ Add Mouse Aim Look");

    int idx = hitTest(e->pos());
    QAction *actDel = nullptr;
    if (idx >= 0) {
        menu.addSeparator();
        actDel = menu.addAction("Delete Selected");
    }

    QPointF ratio = pixelToRatio(e->pos());
    QAction *selected = menu.exec(e->globalPos());

    if (selected == actAddClick) {
        emit canvasDoubleClicked(ratio);
    } else if (selected == actAddJoy) {
        if (m_nodes) {
            KeyNode n;
            n.type = KeyNode::SteerWheel;
            n.comment = "Joystick";
            n.centerPos = ratio;
            m_nodes->append(n);
            emit nodeSelected(m_nodes->size() - 1);
            update();
        }
    } else if (selected == actAddAim) {
        m_hasMouseMove = true;
        m_mouseStartPos = ratio;
        emit mouseAimMoved(ratio);
        update();
    } else if (actDel && selected == actDel && idx >= 0 && m_nodes) {
        m_nodes->removeAt(idx);
        m_selIdx = -1;
        emit nodeSelected(-1);
        update();
    }
}

void KeymapCanvas::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    update();
}
// == KeymapEditor Dialog =========================================

KeymapEditor::KeymapEditor(const QString &serial, QWidget *parent)
    : QDialog(parent, Qt::Window), m_serial(serial) {
    setWindowTitle("Keymap Studio - " + serial);
    resize(1020, 680);
    setMinimumSize(850, 560);

    buildUI();
    applyTheme();
    refreshPresetList();

    QString p = currentFilePath();
    if (QFile::exists(p)) {
        loadJson(p);
    } else {
        QString defP = defaultKeymapDir() + "/gameforpeace.json";
        if (QFile::exists(defP)) loadJson(defP);
    }
}

KeymapEditor::~KeymapEditor() {}

QString KeymapEditor::defaultKeymapDir() const {
    QString d = QCoreApplication::applicationDirPath() + "/keymap";
    QDir().mkpath(d);
    return d;
}

QString KeymapEditor::userKeymapDir() const {
    QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/keymap";
    QDir().mkpath(d);
    return d;
}

QString KeymapEditor::currentFilePath() const {
    if (!m_currentFile.isEmpty()) return m_currentFile;
    QString s = m_serial;
    s.replace(":", "_").replace("/", "_");
    return userKeymapDir() + "/" + s + ".json";
}

void KeymapEditor::applyTheme() {
    setStyleSheet(
        "QDialog { background: #12121f; color: #e0e0e0; font-family: 'Segoe UI', sans-serif; }"
        "QGroupBox { font-weight: bold; border: 1px solid #2a2a44; border-radius: 8px; margin-top: 10px; color: #00d2ff; padding-top: 10px; background: #181829; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 5px; }"
        "QLabel { color: #b0b0cc; font-size: 11px; }"
        "QLineEdit, QComboBox, QDoubleSpinBox { background: #202035; border: 1px solid #353555; border-radius: 5px; padding: 5px 8px; color: #fff; font-size: 12px; }"
        "QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus { border: 1px solid #00d2ff; background: #252542; }"
        "QListWidget { background: #181829; border: 1px solid #2a2a44; border-radius: 6px; color: #eee; padding: 4px; }"
        "QListWidget::item { padding: 6px 8px; border-radius: 4px; margin-bottom: 2px; }"
        "QListWidget::item:selected { background: #00d2ff; color: #000; font-weight: bold; }"
        "QListWidget::item:hover:!selected { background: #25253e; }"
        "QPushButton { background: #25253e; border: 1px solid #3a3a5c; border-radius: 6px; padding: 7px 14px; color: #e0e0f0; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background: #323254; border-color: #00d2ff; color: #00d2ff; }"
        "QPushButton#saveBtn { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00b4d8, stop:1 #0077b6); border: none; color: #fff; font-size: 12px; padding: 9px 18px; }"
        "QPushButton#saveBtn:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00d2ff, stop:1 #0096c7); }"
        "QPushButton#delBtn { background: #381520; border: 1px solid #6b1d2e; color: #ff6b81; }"
        "QPushButton#delBtn:hover { background: #521829; border-color: #ff4757; color: #ff4757; }"
        "QCheckBox { color: #ccc; spacing: 6px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border-radius: 3px; border: 1px solid #444; background: #202035; }"
        "QCheckBox::indicator:checked { background: #00d2ff; border-color: #00d2ff; }"
    );
}

void KeymapEditor::buildUI() {
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    auto *leftContainer = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    auto *topBar = new QHBoxLayout;
    m_orientBtn = new QPushButton("Landscape Mode (16:9)");
    m_orientBtn->setCursor(Qt::PointingHandCursor);
    topBar->addWidget(m_orientBtn);

    m_fileStatusLbl = new QLabel("Profile: (Unsaved)");
    m_fileStatusLbl->setStyleSheet("color: #8888aa; font-style: italic;");
    topBar->addSpacing(10);
    topBar->addWidget(m_fileStatusLbl);
    topBar->addStretch();

    auto *newBtn = new QPushButton("+ New");
    auto *importBtn = new QPushButton("Import");
    topBar->addWidget(newBtn);
    topBar->addWidget(importBtn);
    leftLayout->addLayout(topBar);

    m_canvas = new KeymapCanvas;
    m_canvas->setNodes(&m_nodes);
    m_canvas->setMouseMoveMap(m_hasMouseMove, m_mouseStartPos, m_mouseSpeedX, m_mouseSpeedY, m_smallEyesKey, m_smallEyesPos);
    leftLayout->addWidget(m_canvas, 1);

    auto *bottomInfo = new QLabel("Tips: Double-click canvas to create key | Drag to move | Right-click for options");
    bottomInfo->setStyleSheet("color: #666688; font-size: 10px;");
    leftLayout->addWidget(bottomInfo);

    mainLayout->addWidget(leftContainer, 3);

    auto *rightContainer = new QWidget;
    rightContainer->setFixedWidth(300);
    auto *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    auto *presetGroup = new QGroupBox("Game Profile");
    auto *presetLayout = new QHBoxLayout(presetGroup);
    presetLayout->setContentsMargins(8, 8, 8, 8);
    m_presetCombo = new QComboBox;
    presetLayout->addWidget(m_presetCombo);
    rightLayout->addWidget(presetGroup);

    auto *toolGroup = new QGroupBox("Add Keys & Controls");
    auto *toolLayout = new QGridLayout(toolGroup);
    toolLayout->setContentsMargins(8, 8, 8, 8);
    toolLayout->setSpacing(6);

    auto *addClickBtn = new QPushButton("+ Click Key");
    auto *addTwiceBtn = new QPushButton("+ Double Click");
    auto *addJoyBtn   = new QPushButton("+ WASD Joy");
    auto *addAimBtn   = new QPushButton("+ Mouse Aim");
    auto *addEyeBtn   = new QPushButton("+ Free Look");

    toolLayout->addWidget(addClickBtn, 0, 0);
    toolLayout->addWidget(addTwiceBtn, 0, 1);
    toolLayout->addWidget(addJoyBtn,   1, 0);
    toolLayout->addWidget(addAimBtn,   1, 1);
    toolLayout->addWidget(addEyeBtn,   2, 0, 1, 2);
    rightLayout->addWidget(toolGroup);

    auto *listGroup = new QGroupBox("Mapped Buttons");
    auto *listLayout = new QVBoxLayout(listGroup);
    listLayout->setContentsMargins(8, 8, 8, 8);
    m_nodeList = new QListWidget;
    m_nodeList->setMaximumHeight(120);
    listLayout->addWidget(m_nodeList);
    rightLayout->addWidget(listGroup);

    m_propsGroup = new QGroupBox("Properties");
    auto *propsLayout = new QVBoxLayout(m_propsGroup);
    propsLayout->setContentsMargins(8, 8, 8, 8);
    propsLayout->setSpacing(6);

    auto *formLayout = new QFormLayout;
    formLayout->setSpacing(6);

    m_commentEdit = new QLineEdit;
    m_commentEdit->setPlaceholderText("e.g. Fire, Jump, Reload");
    formLayout->addRow("Name / Tag:", m_commentEdit);

    m_typeCombo = new QComboBox;
    m_typeCombo->addItem("Normal Click", 0);
    m_typeCombo->addItem("Double Click (x2)", 1);
    m_typeCombo->addItem("Multi Click (Burst)", 2);
    m_typeCombo->addItem("WASD Joystick", 3);
    formLayout->addRow("Type:", m_typeCombo);
    propsLayout->addLayout(formLayout);

    m_clickWidget = new QWidget;
    auto *cl = new QFormLayout(m_clickWidget);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(6);
    m_keyRecordBtn = new KeyRecordButton;
    m_switchChk = new QCheckBox("Toggle Aim Mode on Click");
    cl->addRow("Key Shortcut:", m_keyRecordBtn);
    cl->addRow("", m_switchChk);
    propsLayout->addWidget(m_clickWidget);

    m_joyWidget = new QWidget;
    auto *jl = new QFormLayout(m_joyWidget);
    jl->setContentsMargins(0, 0, 0, 0);
    jl->setSpacing(4);
    m_upKeyBtn = new KeyRecordButton; m_upKeyBtn->setRecordedKey("Key_W");
    m_downKeyBtn = new KeyRecordButton; m_downKeyBtn->setRecordedKey("Key_S");
    m_leftKeyBtn = new KeyRecordButton; m_leftKeyBtn->setRecordedKey("Key_A");
    m_rightKeyBtn = new KeyRecordButton; m_rightKeyBtn->setRecordedKey("Key_D");
    m_offsetSpin = new QDoubleSpinBox; m_offsetSpin->setRange(0.02, 0.45); m_offsetSpin->setValue(0.12); m_offsetSpin->setSingleStep(0.02);

    jl->addRow("Up (W):", m_upKeyBtn);
    jl->addRow("Down (S):", m_downKeyBtn);
    jl->addRow("Left (A):", m_leftKeyBtn);
    jl->addRow("Right (D):", m_rightKeyBtn);
    jl->addRow("Radius:", m_offsetSpin);
    propsLayout->addWidget(m_joyWidget);
    m_joyWidget->hide();

    m_aimWidget = new QWidget;
    auto *al = new QFormLayout(m_aimWidget);
    al->setContentsMargins(0, 0, 0, 0);
    al->setSpacing(4);
    m_switchKeyBtn = new KeyRecordButton; m_switchKeyBtn->setRecordedKey(m_switchKey);
    m_aimSpeedXSpin = new QDoubleSpinBox; m_aimSpeedXSpin->setRange(0.1, 15.0); m_aimSpeedXSpin->setValue(m_mouseSpeedX); m_aimSpeedXSpin->setSingleStep(0.25);
    m_aimSpeedYSpin = new QDoubleSpinBox; m_aimSpeedYSpin->setRange(0.1, 15.0); m_aimSpeedYSpin->setValue(m_mouseSpeedY); m_aimSpeedYSpin->setSingleStep(0.25);
    al->addRow("Toggle Key:", m_switchKeyBtn);
    al->addRow("Speed X:", m_aimSpeedXSpin);
    al->addRow("Speed Y:", m_aimSpeedYSpin);
    propsLayout->addWidget(m_aimWidget);
    m_aimWidget->hide();

    m_eyeWidget = new QWidget;
    auto *el = new QFormLayout(m_eyeWidget);
    el->setContentsMargins(0, 0, 0, 0);
    el->setSpacing(4);
    m_eyeKeyBtn = new KeyRecordButton; m_eyeKeyBtn->setRecordedKey(m_smallEyesKey);
    el->addRow("Eye Key:", m_eyeKeyBtn);
    propsLayout->addWidget(m_eyeWidget);
    m_eyeWidget->hide();

    auto *propBtns = new QHBoxLayout;
    m_applyPropsBtn = new QPushButton("Apply");
    m_duplicateBtn  = new QPushButton("Duplicate");
    m_deleteBtn     = new QPushButton("Delete");
    m_deleteBtn->setObjectName("delBtn");

    propBtns->addWidget(m_applyPropsBtn);
    propBtns->addWidget(m_duplicateBtn);
    propBtns->addWidget(m_deleteBtn);
    propsLayout->addLayout(propBtns);

    rightLayout->addWidget(m_propsGroup);
    rightLayout->addStretch();

    auto *actionLayout = new QVBoxLayout;
    auto *saveBtn = new QPushButton("Save & Apply to Device");
    saveBtn->setObjectName("saveBtn");
    saveBtn->setCursor(Qt::PointingHandCursor);

    auto *subActions = new QHBoxLayout;
    auto *saveAsBtn = new QPushButton("Save As...");
    auto *closeBtn  = new QPushButton("Close");
    subActions->addWidget(saveAsBtn);
    subActions->addWidget(closeBtn);

    actionLayout->addWidget(saveBtn);
    actionLayout->addLayout(subActions);
    rightLayout->addLayout(actionLayout);

    mainLayout->addWidget(rightContainer);

    connect(m_canvas, &KeymapCanvas::nodeSelected, this, &KeymapEditor::onNodeSelected);
    connect(m_canvas, &KeymapCanvas::nodeMoved, this, &KeymapEditor::onNodeMoved);
    connect(m_canvas, &KeymapCanvas::mouseAimMoved, this, &KeymapEditor::onMouseAimMoved);
    connect(m_canvas, &KeymapCanvas::smallEyesMoved, this, &KeymapEditor::onSmallEyesMoved);
    connect(m_canvas, &KeymapCanvas::canvasDoubleClicked, this, &KeymapEditor::onCanvasDoubleClicked);

    connect(m_nodeList, &QListWidget::currentRowChanged, this, &KeymapEditor::onNodeSelected);
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KeymapEditor::onLoadPreset);

    connect(addClickBtn, &QPushButton::clicked, this, &KeymapEditor::onAddClick);
    connect(addTwiceBtn, &QPushButton::clicked, this, &KeymapEditor::onAddClickTwice);
    connect(addJoyBtn,   &QPushButton::clicked, this, &KeymapEditor::onAddJoystick);
    connect(addAimBtn,   &QPushButton::clicked, this, &KeymapEditor::onAddMouseAim);
    connect(addEyeBtn,   &QPushButton::clicked, this, &KeymapEditor::onAddSmallEyes);

    connect(m_applyPropsBtn, &QPushButton::clicked, this, &KeymapEditor::onApplyProps);
    connect(m_duplicateBtn,  &QPushButton::clicked, this, &KeymapEditor::onDuplicateSelected);
    connect(m_deleteBtn,     &QPushButton::clicked, this, &KeymapEditor::onDeleteSelected);

    connect(saveBtn,   &QPushButton::clicked, this, &KeymapEditor::onSaveAndApply);
    connect(saveAsBtn, &QPushButton::clicked, this, &KeymapEditor::onSaveAs);
    connect(newBtn,    &QPushButton::clicked, this, &KeymapEditor::onNewLayout);
    connect(importBtn, &QPushButton::clicked, this, &KeymapEditor::onImportFile);
    connect(closeBtn,  &QPushButton::clicked, this, &QDialog::accept);
    connect(m_orientBtn, &QPushButton::clicked, this, &KeymapEditor::onToggleOrientation);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int i) {
        m_clickWidget->setVisible(i >= 0 && i <= 2);
        m_joyWidget->setVisible(i == 3);
        m_aimWidget->hide();
        m_eyeWidget->hide();
    });

    connect(m_keyRecordBtn, &KeyRecordButton::keyChanged, [this](const QString &k) {
        if (m_selIdx >= 0 && m_selIdx < m_nodes.size()) {
            m_nodes[m_selIdx].key = k;
            populateList();
            m_canvas->update();
        }
    });

    connect(m_switchKeyBtn, &KeyRecordButton::keyChanged, [this](const QString &k) {
        m_switchKey = k;
    });

    connect(m_eyeKeyBtn, &KeyRecordButton::keyChanged, [this](const QString &k) {
        m_smallEyesKey = k;
        m_canvas->setMouseMoveMap(m_hasMouseMove, m_mouseStartPos, m_mouseSpeedX, m_mouseSpeedY, m_smallEyesKey, m_smallEyesPos);
    });
}

void KeymapEditor::onToggleOrientation() {
    if (m_canvas->orientation() == KeymapCanvas::Landscape) {
        m_canvas->setOrientation(KeymapCanvas::Portrait);
        m_orientBtn->setText("Portrait Mode (9:16)");
    } else {
        m_canvas->setOrientation(KeymapCanvas::Landscape);
        m_orientBtn->setText("Landscape Mode (16:9)");
    }
}

void KeymapEditor::refreshPresetList() {
    m_presetCombo->blockSignals(true);
    m_presetCombo->clear();
    m_presetCombo->addItem("-- Select Game Profile --", "");

    QSet<QString> added;
    auto scanDir = [this, &added](const QString &dirPath) {
        QDir dir(dirPath);
        QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
        for (const QString &f : files) {
            QString name = f.left(f.length() - 5);
            if (!added.contains(name)) {
                added.insert(name);
                m_presetCombo->addItem(name, dir.absoluteFilePath(f));
            }
        }
    };

    scanDir(defaultKeymapDir());
    scanDir(userKeymapDir());
    m_presetCombo->blockSignals(false);
}

void KeymapEditor::populateList() {
    m_nodeList->blockSignals(true);
    m_nodeList->clear();

    if (m_hasMouseMove) {
        auto *item = new QListWidgetItem("Aim Look (Mouse Look)");
        item->setData(Qt::UserRole, -2);
        m_nodeList->addItem(item);

        if (m_hasSmallEyes) {
            auto *eyeItem = new QListWidgetItem("Free Look (" + friendlyKeyName(m_smallEyesKey) + ")");
            eyeItem->setData(Qt::UserRole, -3);
            m_nodeList->addItem(eyeItem);
        }
    }

    for (int i = 0; i < m_nodes.size(); ++i) {
        const KeyNode &n = m_nodes.at(i);
        QString text;
        if (n.type == KeyNode::SteerWheel) {
            text = QString("WASD (%1)").arg(n.comment.isEmpty() ? "Movement" : n.comment);
        } else {
            QString k = friendlyKeyName(n.key);
            QString t = (n.type == KeyNode::ClickTwice) ? "[x2] " : "";
            text = QString("%1%2 (%3)").arg(t, k, n.comment.isEmpty() ? "Button" : n.comment);
        }
        auto *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, i);
        m_nodeList->addItem(item);
    }
    m_nodeList->blockSignals(false);
}

void KeymapEditor::loadPropsToUI(int idx) {
    if (idx == -2) {
        m_commentEdit->setText("Aim Look (Camera)");
        m_clickWidget->hide();
        m_joyWidget->hide();
        m_eyeWidget->hide();
        m_aimWidget->show();
        m_switchKeyBtn->setRecordedKey(m_switchKey);
        m_aimSpeedXSpin->setValue(m_mouseSpeedX);
        m_aimSpeedYSpin->setValue(m_mouseSpeedY);
        return;
    }
    if (idx == -3) {
        m_commentEdit->setText("Free Look (Small Eye)");
        m_clickWidget->hide();
        m_joyWidget->hide();
        m_aimWidget->hide();
        m_eyeWidget->show();
        m_eyeKeyBtn->setRecordedKey(m_smallEyesKey);
        return;
    }

    if (idx < 0 || idx >= m_nodes.size()) {
        m_propsGroup->setEnabled(false);
        return;
    }

    m_propsGroup->setEnabled(true);
    const KeyNode &n = m_nodes.at(idx);
    m_commentEdit->setText(n.comment);

    if (n.type == KeyNode::SteerWheel) {
        m_typeCombo->setCurrentIndex(3);
        m_upKeyBtn->setRecordedKey(n.upKey);
        m_downKeyBtn->setRecordedKey(n.downKey);
        m_leftKeyBtn->setRecordedKey(n.leftKey);
        m_rightKeyBtn->setRecordedKey(n.rightKey);
        m_offsetSpin->setValue(n.upOffset);
        m_clickWidget->hide();
        m_joyWidget->show();
        m_aimWidget->hide();
        m_eyeWidget->hide();
    } else {
        m_typeCombo->setCurrentIndex(n.type == KeyNode::ClickTwice ? 1 :
                                     n.type == KeyNode::ClickMulti ? 2 : 0);
        m_keyRecordBtn->setRecordedKey(n.key);
        m_switchChk->setChecked(n.switchMap);
        m_clickWidget->show();
        m_joyWidget->hide();
        m_aimWidget->hide();
        m_eyeWidget->hide();
    }
}

void KeymapEditor::savePropsFromUI(int idx) {
    if (idx == -2) {
        m_switchKey = m_switchKeyBtn->recordedKey();
        m_mouseSpeedX = m_aimSpeedXSpin->value();
        m_mouseSpeedY = m_aimSpeedYSpin->value();
        m_canvas->setMouseMoveMap(m_hasMouseMove, m_mouseStartPos, m_mouseSpeedX, m_mouseSpeedY, m_smallEyesKey, m_smallEyesPos);
        return;
    }
    if (idx == -3) {
        m_smallEyesKey = m_eyeKeyBtn->recordedKey();
        m_canvas->setMouseMoveMap(m_hasMouseMove, m_mouseStartPos, m_mouseSpeedX, m_mouseSpeedY, m_smallEyesKey, m_smallEyesPos);
        return;
    }

    if (idx < 0 || idx >= m_nodes.size()) return;

    KeyNode &n = m_nodes[idx];
    n.comment = m_commentEdit->text().trimmed();

    int typeIdx = m_typeCombo->currentIndex();
    if (typeIdx == 3) {
        n.type = KeyNode::SteerWheel;
        n.upKey = m_upKeyBtn->recordedKey();
        n.downKey = m_downKeyBtn->recordedKey();
        n.leftKey = m_leftKeyBtn->recordedKey();
        n.rightKey = m_rightKeyBtn->recordedKey();
        double off = m_offsetSpin->value();
        n.upOffset = n.downOffset = n.leftOffset = n.rightOffset = off;
    } else {
        n.type = (typeIdx == 1) ? KeyNode::ClickTwice :
                 (typeIdx == 2) ? KeyNode::ClickMulti : KeyNode::Click;
        n.key = m_keyRecordBtn->recordedKey();
        n.switchMap = m_switchChk->isChecked();
    }
}

void KeymapEditor::onNodeSelected(int idx) {
    m_selIdx = idx;
    m_canvas->setSelectedIndex(idx);
    loadPropsToUI(idx);

    for (int i = 0; i < m_nodeList->count(); ++i) {
        if (m_nodeList->item(i)->data(Qt::UserRole).toInt() == idx) {
            m_nodeList->blockSignals(true);
            m_nodeList->setCurrentRow(i);
            m_nodeList->blockSignals(false);
            break;
        }
    }
}

void KeymapEditor::onNodeMoved(int, QPointF) {}
void KeymapEditor::onMouseAimMoved(QPointF r) { m_mouseStartPos = r; }
void KeymapEditor::onSmallEyesMoved(QPointF r) { m_smallEyesPos = r; }

void KeymapEditor::onCanvasDoubleClicked(QPointF ratio) {
    KeyNode n;
    n.type = KeyNode::Click;
    n.comment = QString("Button %1").arg(m_nodes.size() + 1);
    n.key = "Key_Space";
    n.pos = ratio;
    m_nodes.append(n);

    populateList();
    onNodeSelected(m_nodes.size() - 1);
    m_canvas->update();
}

void KeymapEditor::onAddClick() {
    onCanvasDoubleClicked(QPointF(0.5, 0.5));
}

void KeymapEditor::onAddClickTwice() {
    KeyNode n;
    n.type = KeyNode::ClickTwice;
    n.comment = "Double Tap";
    n.key = "Key_E";
    n.pos = QPointF(0.6, 0.5);
    m_nodes.append(n);
    populateList();
    onNodeSelected(m_nodes.size() - 1);
    m_canvas->update();
}

void KeymapEditor::onAddJoystick() {
    KeyNode n;
    n.type = KeyNode::SteerWheel;
    n.comment = "WASD Movement";
    n.centerPos = QPointF(0.18, 0.72);
    m_nodes.append(n);
    populateList();
    onNodeSelected(m_nodes.size() - 1);
    m_canvas->update();
}

void KeymapEditor::onAddMouseAim() {
    m_hasMouseMove = true;
    m_canvas->setMouseMoveMap(m_hasMouseMove, m_mouseStartPos, m_mouseSpeedX, m_mouseSpeedY, m_smallEyesKey, m_smallEyesPos);
    populateList();
    onNodeSelected(-2);
    m_canvas->update();
}

void KeymapEditor::onAddSmallEyes() {
    m_hasSmallEyes = true;
    m_canvas->setMouseMoveMap(m_hasMouseMove, m_mouseStartPos, m_mouseSpeedX, m_mouseSpeedY, m_smallEyesKey, m_smallEyesPos);
    populateList();
    onNodeSelected(-3);
    m_canvas->update();
}

void KeymapEditor::onDeleteSelected() {
    if (m_selIdx == -2) {
        m_hasMouseMove = false;
        m_canvas->setMouseMoveMap(false, m_mouseStartPos, m_mouseSpeedX, m_mouseSpeedY, m_smallEyesKey, m_smallEyesPos);
        m_selIdx = -1;
    } else if (m_selIdx == -3) {
        m_hasSmallEyes = false;
        m_canvas->setMouseMoveMap(m_hasMouseMove, m_mouseStartPos, m_mouseSpeedX, m_mouseSpeedY, m_smallEyesKey, m_smallEyesPos);
        m_selIdx = -1;
    } else if (m_selIdx >= 0 && m_selIdx < m_nodes.size()) {
        m_nodes.removeAt(m_selIdx);
        m_selIdx = -1;
    }
    populateList();
    m_canvas->setSelectedIndex(m_selIdx);
    loadPropsToUI(m_selIdx);
    m_canvas->update();
}

void KeymapEditor::onDuplicateSelected() {
    if (m_selIdx >= 0 && m_selIdx < m_nodes.size()) {
        KeyNode copy = m_nodes.at(m_selIdx);
        copy.pos += QPointF(0.04, 0.04);
        copy.comment += " (Copy)";
        m_nodes.append(copy);
        populateList();
        onNodeSelected(m_nodes.size() - 1);
        m_canvas->update();
    }
}

void KeymapEditor::onApplyProps() {
    if (m_selIdx != -1) {
        savePropsFromUI(m_selIdx);
        populateList();
        m_canvas->update();
    }
}

void KeymapEditor::onSaveAndApply() {
    if (m_selIdx != -1) savePropsFromUI(m_selIdx);

    QString path = currentFilePath();
    if (saveJson(path)) {
        applyToDevice();
        QMessageBox::information(this, "Keymap Applied", "Keymap saved and applied live to your device!");
    } else {
        QMessageBox::warning(this, "Save Failed", "Could not write keymap file to:\n" + path);
    }
}

void KeymapEditor::onSaveAs() {
    bool ok;
    QString name = QInputDialog::getText(this, "Save Profile As", "Enter profile name:", QLineEdit::Normal, "MyCustomGame", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString path = userKeymapDir() + "/" + name.trimmed() + ".json";
    if (saveJson(path)) {
        m_currentFile = path;
        refreshPresetList();
        applyToDevice();
        QMessageBox::information(this, "Profile Saved", "Profile saved as: " + name);
    }
}

void KeymapEditor::onLoadPreset(int index) {
    if (index <= 0) return;
    QString path = m_presetCombo->itemData(index).toString();
    if (!path.isEmpty() && QFile::exists(path)) {
        loadJson(path);
    }
}

void KeymapEditor::onImportFile() {
    QString p = QFileDialog::getOpenFileName(this, "Import Keymap JSON", defaultKeymapDir(), "JSON Keymaps (*.json)");
    if (!p.isEmpty()) loadJson(p);
}

void KeymapEditor::onNewLayout() {
    m_nodes.clear();
    m_hasMouseMove = true;
    m_mouseStartPos = QPointF(0.55, 0.5);
    m_hasSmallEyes = false;
    m_selIdx = -1;
    m_currentFile.clear();
    m_fileStatusLbl->setText("Profile: (New Layout)");

    KeyNode wasd;
    wasd.type = KeyNode::SteerWheel;
    wasd.comment = "WASD";
    wasd.centerPos = QPointF(0.18, 0.72);
    m_nodes.append(wasd);

    populateList();
    m_canvas->setSelectedIndex(-1);
    m_canvas->setMouseMoveMap(m_hasMouseMove, m_mouseStartPos, m_mouseSpeedX, m_mouseSpeedY, m_smallEyesKey, m_smallEyesPos);
    m_canvas->update();
}

bool KeymapEditor::loadJson(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    QJsonObject root = doc.object();
    m_switchKey = root["switchKey"].toString("Key_QuoteLeft");

    if (root.contains("mouseMoveMap")) {
        m_hasMouseMove = true;
        QJsonObject mm = root["mouseMoveMap"].toObject();
        QJsonObject sp = mm["startPos"].toObject();
        m_mouseStartPos = QPointF(sp["x"].toDouble(0.55), sp["y"].toDouble(0.5));
        m_mouseSpeedX   = mm["speedRatioX"].toDouble(3.0);
        m_mouseSpeedY   = mm["speedRatioY"].toDouble(1.5);

        if (mm.contains("smallEyes")) {
            m_hasSmallEyes = true;
            QJsonObject se = mm["smallEyes"].toObject();
            m_smallEyesKey = se["key"].toString("Key_Alt");
            QJsonObject sep = se["pos"].toObject();
            m_smallEyesPos = QPointF(sep["x"].toDouble(0.8), sep["y"].toDouble(0.3));
        } else {
            m_hasSmallEyes = false;
        }
    } else {
        m_hasMouseMove = false;
        m_hasSmallEyes = false;
    }

    m_nodes.clear();
    QJsonArray arr = root["keyMapNodes"].toArray();
    for (const QJsonValue &v : arr) {
        m_nodes.append(KeyNode::fromJson(v.toObject()));
    }

    m_currentFile = path;
    QFileInfo fi(path);
    m_fileStatusLbl->setText("Profile: " + fi.fileName());

    m_canvas->setMouseMoveMap(m_hasMouseMove, m_mouseStartPos, m_mouseSpeedX, m_mouseSpeedY, m_smallEyesKey, m_smallEyesPos);
    populateList();
    onNodeSelected(-1);
    m_canvas->update();
    return true;
}

bool KeymapEditor::saveJson(const QString &path) {
    QJsonObject root;
    root["switchKey"] = m_switchKey;

    if (m_hasMouseMove) {
        QJsonObject mm;
        QJsonObject sp;
        sp["x"] = m_mouseStartPos.x();
        sp["y"] = m_mouseStartPos.y();
        mm["startPos"]    = sp;
        mm["speedRatioX"] = m_mouseSpeedX;
        mm["speedRatioY"] = m_mouseSpeedY;
        mm["speedRatio"]  = 10;

        if (m_hasSmallEyes) {
            QJsonObject se;
            se["comment"]   = "Free Look";
            se["type"]      = "KMT_CLICK";
            se["key"]       = m_smallEyesKey;
            QJsonObject sep;
            sep["x"] = m_smallEyesPos.x();
            sep["y"] = m_smallEyesPos.y();
            se["pos"]       = sep;
            se["switchMap"] = false;
            mm["smallEyes"] = se;
        }
        root["mouseMoveMap"] = mm;
    }

    QJsonArray arr;
    for (const KeyNode &n : m_nodes) {
        arr.append(n.toJson());
    }
    root["keyMapNodes"] = arr;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();

    m_currentFile = path;
    QFileInfo fi(path);
    m_fileStatusLbl->setText("Profile: " + fi.fileName());
    return true;
}

void KeymapEditor::applyToDevice() {
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) return;

    QFile f(currentFilePath());
    if (!f.open(QIODevice::ReadOnly)) return;

    QString jsonStr = QString::fromUtf8(f.readAll());
    f.close();

    device->updateScript(jsonStr);
}

void KeymapEditor::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
        onDeleteSelected();
        e->accept();
        return;
    }
    QDialog::keyPressEvent(e);
}