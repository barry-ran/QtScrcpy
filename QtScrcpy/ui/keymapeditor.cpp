#include "keymapeditor.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonDocument>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QStandardPaths>
#include <QtMath>

#include "../QtScrcpyCore/include/QtScrcpyCore.h"

// ═══════════════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════════════
QString qtKeyToString(int key) { return QKeySequence(key).toString(); }

QString friendlyKeyName(const QString &s) {
    if (s.isEmpty()) return "?";
    QString t = s;
    t.remove("Key_");
    if (t == "QuoteLeft") return "`";
    if (t == "Space")     return "SPC";
    if (t == "Return")    return "ENTER";
    if (t == "Shift")     return "SHIFT";
    if (t == "Control")   return "CTRL";
    if (t == "Alt")       return "ALT";
    if (t == "Escape")    return "ESC";
    return t.length() == 1 ? t.toUpper() : t;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  KeyNode serialization
// ═══════════════════════════════════════════════════════════════════════════════
static QString typeToStr(KeyNode::Type t) {
    switch (t) {
    case KeyNode::ClickTwice: return "KMT_CLICK_TWICE";
    case KeyNode::ClickMulti: return "KMT_CLICK_MULTI";
    case KeyNode::SteerWheel: return "KMT_STEER_WHEEL";
    case KeyNode::Drag:       return "KMT_DRAG";
    default:                  return "KMT_CLICK";
    }
}

static KeyNode::Type strToType(const QString &s) {
    if (s == "KMT_CLICK_TWICE") return KeyNode::ClickTwice;
    if (s == "KMT_CLICK_MULTI") return KeyNode::ClickMulti;
    if (s == "KMT_STEER_WHEEL") return KeyNode::SteerWheel;
    if (s == "KMT_DRAG")        return KeyNode::Drag;
    return KeyNode::Click;
}

QJsonObject KeyNode::toJson() const {
    QJsonObject o;
    o["comment"] = comment;
    o["type"]    = typeToStr(type);
    if (type == SteerWheel) {
        QJsonObject cp; cp["x"] = centerPos.x(); cp["y"] = centerPos.y();
        o["centerPos"]   = cp;
        o["leftOffset"]  = leftOffset;
        o["rightOffset"] = rightOffset;
        o["upOffset"]    = upOffset;
        o["downOffset"]  = downOffset;
        o["leftKey"]     = leftKey;
        o["rightKey"]    = rightKey;
        o["upKey"]       = upKey;
        o["downKey"]     = downKey;
    } else if (type == Drag) {
        QJsonObject sp; sp["x"] = startPos.x(); sp["y"] = startPos.y();
        QJsonObject ep; ep["x"] = endPos.x();   ep["y"] = endPos.y();
        o["startPos"] = sp; o["endPos"] = ep;
    } else {
        QJsonObject p; p["x"] = pos.x(); p["y"] = pos.y();
        o["key"] = key; o["pos"] = p; o["switchMap"] = switchMap;
    }
    return o;
}

KeyNode KeyNode::fromJson(const QJsonObject &o) {
    KeyNode n;
    n.comment = o["comment"].toString();
    n.type    = strToType(o["type"].toString());
    if (n.type == SteerWheel) {
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
    } else if (n.type == Drag) {
        QJsonObject sp = o["startPos"].toObject(); QJsonObject ep = o["endPos"].toObject();
        n.startPos = QPointF(sp["x"].toDouble(0.3), sp["y"].toDouble(0.5));
        n.endPos   = QPointF(ep["x"].toDouble(0.7), ep["y"].toDouble(0.5));
    } else {
        QJsonObject p = o["pos"].toObject();
        n.key       = o["key"].toString("Key_Space");
        n.pos       = QPointF(p["x"].toDouble(0.5), p["y"].toDouble(0.5));
        n.switchMap = o["switchMap"].toBool(false);
    }
    return n;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  KeyRecordButton
// ═══════════════════════════════════════════════════════════════════════════════
KeyRecordButton::KeyRecordButton(QWidget *parent) : QPushButton(parent) {
    setFocusPolicy(Qt::StrongFocus);
    updateButtonText();
}
void KeyRecordButton::setRecordedKey(const QString &k) { m_keyName=k; m_recording=false; updateButtonText(); }
void KeyRecordButton::mousePressEvent(QMouseEvent *e) {
    if (e->button()==Qt::LeftButton) { m_recording=true; setText("[ Press key... ]"); setFocus(); grabKeyboard(); }
}
void KeyRecordButton::keyPressEvent(QKeyEvent *e) {
    if (!m_recording) { QPushButton::keyPressEvent(e); return; }
    int k=e->key();
    if (k==Qt::Key_Escape) { m_recording=false; updateButtonText(); releaseKeyboard(); return; }
    m_keyName="Key_"+QKeySequence(k).toString();
    m_recording=false; updateButtonText(); releaseKeyboard(); emit keyChanged(m_keyName);
}
void KeyRecordButton::focusOutEvent(QFocusEvent *e) {
    if (m_recording) { m_recording=false; updateButtonText(); releaseKeyboard(); }
    QPushButton::focusOutEvent(e);
}
void KeyRecordButton::updateButtonText() { setText(friendlyKeyName(m_keyName)); }

// ═══════════════════════════════════════════════════════════════════════════════
//  KeymapOverlay
// ═══════════════════════════════════════════════════════════════════════════════
static const int NODE_R = 22;
static const int JOY_R  = 40;

KeymapOverlay::KeymapOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void KeymapOverlay::setNodes(QList<KeyNode> *nodes) { m_nodes=nodes; update(); }
void KeymapOverlay::setSelectedIndex(int idx)        { m_selIdx=idx;  update(); }

void KeymapOverlay::setMouseMoveMap(bool en, QPointF sp, double sx, double sy,
                                    const QString &ek, QPointF ep) {
    m_hasMouseMove=en; m_mouseStartPos=sp; m_speedX=sx; m_speedY=sy; m_eyeKey=ek; m_eyePos=ep; update();
}

QPoint  KeymapOverlay::ratioToPixel(QPointF r) const { return QPoint(qRound(r.x()*width()), qRound(r.y()*height())); }
QPointF KeymapOverlay::pixelToRatio(QPoint p)  const { return QPointF(qreal(p.x())/width(), qreal(p.y())/height()); }

int KeymapOverlay::hitTest(QPoint p) const {
    if (m_hasMouseMove) {
        if ((ratioToPixel(m_mouseStartPos)-p).manhattanLength() < NODE_R+8) return -2;
        if ((ratioToPixel(m_eyePos)-p).manhattanLength()        < NODE_R+8) return -3;
    }
    if (!m_nodes) return -1;
    for (int i=m_nodes->size()-1; i>=0; --i) {
        const KeyNode &n=m_nodes->at(i);
        QPoint c=(n.type==KeyNode::SteerWheel)?ratioToPixel(n.centerPos):ratioToPixel(n.pos);
        int r=(n.type==KeyNode::SteerWheel)?JOY_R:NODE_R;
        if ((c-p).manhattanLength()<r+8) return i;
    }
    return -1;
}

void KeymapOverlay::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Top hint bar
    p.fillRect(0,0,width(),26,QColor(0,0,0,170));
    p.setPen(QColor(0,200,255));
    p.setFont(QFont("Arial",8));
    p.drawText(QRect(0,0,width(),26), Qt::AlignCenter,
               "Keymap Edit Mode  |  Double-click canvas to add key  |  Drag to move  |  Right-click for menu");

    if (!m_nodes) return;

    for (int i=0; i<m_nodes->size(); ++i) {
        const KeyNode &n=m_nodes->at(i);
        bool sel=(i==m_selIdx);
        if (n.type==KeyNode::SteerWheel) {
            QPoint c=ratioToPixel(n.centerPos);
            p.setPen(QPen(sel?QColor(0,200,255):QColor(100,200,255), sel?3:2));
            p.setBrush(QColor(30,60,120,130));
            p.drawEllipse(c,JOY_R,JOY_R);
            p.setBrush(QColor(80,140,200,190));
            p.setPen(Qt::NoPen);
            p.drawEllipse(c,12,12);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial",8,QFont::Bold));
            p.drawText(QRect(c.x()-20,c.y()-JOY_R-14,40,13),Qt::AlignCenter,friendlyKeyName(n.upKey));
            p.drawText(QRect(c.x()-20,c.y()+JOY_R+1, 40,13),Qt::AlignCenter,friendlyKeyName(n.downKey));
            p.drawText(QRect(c.x()-JOY_R-28,c.y()-6,28,13),Qt::AlignRight|Qt::AlignVCenter,friendlyKeyName(n.leftKey));
            p.drawText(QRect(c.x()+JOY_R+2, c.y()-6,28,13),Qt::AlignLeft|Qt::AlignVCenter,friendlyKeyName(n.rightKey));
            if (!n.comment.isEmpty()) {
                p.setFont(QFont("Arial",7)); p.setPen(QColor(180,220,255));
                p.drawText(QRect(c.x()-40,c.y()+JOY_R+16,80,12),Qt::AlignCenter,n.comment);
            }
        } else {
            QPoint c=ratioToPixel(n.pos);
            QColor fill=(n.type==KeyNode::ClickTwice)?QColor(100,50,150,180):QColor(30,80,160,180);
            p.setPen(QPen(sel?QColor(0,220,255):QColor(100,180,255),sel?3:2));
            p.setBrush(fill);
            p.drawEllipse(c,NODE_R,NODE_R);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial",9,QFont::Bold));
            p.drawText(QRect(c.x()-NODE_R,c.y()-NODE_R,NODE_R*2,NODE_R*2),Qt::AlignCenter,friendlyKeyName(n.key));
            if (!n.comment.isEmpty()) {
                p.setFont(QFont("Arial",7)); p.setPen(QColor(200,230,255));
                p.drawText(QRect(c.x()-30,c.y()+NODE_R+2,60,12),Qt::AlignCenter,n.comment);
            }
        }
    }

    // Aim & Free Look
    if (m_hasMouseMove) {
        // Mouse Aim
        QPoint c=ratioToPixel(m_mouseStartPos); bool sel=(m_selIdx==-2);
        p.setPen(QPen(sel?QColor(0,255,180):QColor(80,220,160),sel?3:2));
        p.setBrush(QColor(0,120,80,140));
        p.drawEllipse(c,NODE_R+4,NODE_R+4);
        p.setPen(QPen(Qt::white,1));
        p.drawLine(c.x()-9,c.y(),c.x()+9,c.y()); p.drawLine(c.x(),c.y()-9,c.x(),c.y()+9);
        p.setPen(QColor(180,255,220)); p.setFont(QFont("Arial",7));
        p.drawText(QRect(c.x()-30,c.y()+NODE_R+8,60,12),Qt::AlignCenter,"Aim Look");

        // Free Look
        QPoint ec=ratioToPixel(m_eyePos); bool esel=(m_selIdx==-3);
        p.setPen(QPen(esel?QColor(0,255,180):QColor(80,220,160),esel?3:2));
        p.setBrush(QColor(0,80,60,140));
        p.drawEllipse(ec,NODE_R,NODE_R);
        p.setPen(Qt::white); p.setFont(QFont("Arial",8,QFont::Bold));
        p.drawText(QRect(ec.x()-NODE_R,ec.y()-NODE_R,NODE_R*2,NODE_R*2),Qt::AlignCenter,friendlyKeyName(m_eyeKey));
        p.setFont(QFont("Arial",7)); p.setPen(QColor(200,255,220));
        p.drawText(QRect(ec.x()-30,ec.y()+NODE_R+2,60,12),Qt::AlignCenter,"Free Look");
    }
}

void KeymapOverlay::mousePressEvent(QMouseEvent *e) {
    if (e->button()==Qt::LeftButton) {
        int hit=hitTest(e->pos());
        m_selIdx=hit; emit nodeSelected(hit);
        if (hit!=-1) {
            m_drag=true;
            QPoint nc;
            if      (hit>=0 && m_nodes && hit<m_nodes->size()) {
                const KeyNode &n=m_nodes->at(hit);
                nc=(n.type==KeyNode::SteerWheel)?ratioToPixel(n.centerPos):ratioToPixel(n.pos);
            } else if (hit==-2) nc=ratioToPixel(m_mouseStartPos);
            else if  (hit==-3) nc=ratioToPixel(m_eyePos);
            m_dragOffset=e->pos()-nc;
        }
        update();
    }
}

void KeymapOverlay::mouseMoveEvent(QMouseEvent *e) {
    if (m_drag && m_selIdx!=-1) {
        QPointF r=pixelToRatio(e->pos()-m_dragOffset);
        r.setX(qBound(0.0,r.x(),1.0)); r.setY(qBound(0.0,r.y(),1.0));
        if (m_selIdx>=0 && m_nodes && m_selIdx<m_nodes->size()) {
            KeyNode &n=(*m_nodes)[m_selIdx];
            if (n.type==KeyNode::SteerWheel) n.centerPos=r; else n.pos=r;
            emit nodeMoved(m_selIdx,r);
        } else if (m_selIdx==-2) { m_mouseStartPos=r; emit mouseAimMoved(r); }
        else if   (m_selIdx==-3) { m_eyePos=r;        emit smallEyesMoved(r); }
        update();
    }
}

void KeymapOverlay::mouseReleaseEvent(QMouseEvent *)  { m_drag=false; }

void KeymapOverlay::mouseDoubleClickEvent(QMouseEvent *e) {
    if (e->button()==Qt::LeftButton && hitTest(e->pos())==-1)
        emit overlayDoubleClicked(pixelToRatio(e->pos()));
}

void KeymapOverlay::contextMenuEvent(QContextMenuEvent *e) {
    int hit=hitTest(e->pos()); if (hit==-1) return;
    m_selIdx=hit; emit nodeSelected(hit); update();
    QMenu menu(this);
    menu.setStyleSheet("QMenu{background:#1e2a38;color:#d0e8ff;border:1px solid #3a6090;}"
                       "QMenu::item:selected{background:#2a4a70;}");
    QAction *del=menu.addAction("Delete");
    QAction *dup=menu.addAction("Duplicate");
    QAction *act=menu.exec(e->globalPos());
    if      (act==del) emit requestDelete(hit);
    else if (act==dup) emit requestDelete(-999); // duplicate signal via controller
}

// ═══════════════════════════════════════════════════════════════════════════════
//  KeymapSidePanel
// ═══════════════════════════════════════════════════════════════════════════════
KeymapSidePanel::KeymapSidePanel(QWidget *parent) : QFrame(parent) {
    setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedWidth(225);
    buildUI();
}

void KeymapSidePanel::buildUI() {
    setStyleSheet(R"(
        QFrame{background:#0f1922;border:1px solid #243548;border-radius:8px;}
        QLabel{color:#90bce0;font-size:11px;background:transparent;border:none;}
        QPushButton{background:#16304a;color:#b8deff;border:1px solid #2a5080;border-radius:5px;padding:4px 6px;font-size:11px;}
        QPushButton:hover{background:#1e4a70;}
        QPushButton:pressed{background:#0a1828;}
        QGroupBox{color:#6a9cc8;font-size:10px;border:1px solid #1e3a58;border-radius:6px;margin-top:8px;padding-top:6px;background:transparent;}
        QGroupBox::title{subcontrol-origin:margin;left:8px;color:#508ac0;}
        QLineEdit,QDoubleSpinBox,QComboBox{background:#0a1522;color:#b8deff;border:1px solid #1e3a58;border-radius:4px;padding:2px 4px;font-size:11px;}
        QCheckBox{color:#90bce0;background:transparent;border:none;}
        QScrollArea{border:none;background:transparent;}
        QScrollBar:vertical{width:5px;background:#0a1522;}
        QScrollBar::handle:vertical{background:#1e3a58;border-radius:2px;}
    )");

    auto *root=new QVBoxLayout(this);
    root->setContentsMargins(6,6,6,6); root->setSpacing(4);

    // Title row
    auto *tr=new QHBoxLayout();
    auto *tl=new QLabel("\xF0\x9F\x8E\xAE Keymap Studio");
    tl->setStyleSheet("color:#00ccff;font-weight:bold;font-size:12px;background:transparent;border:none;");
    m_fileLabel=new QLabel("(New Layout)");
    m_fileLabel->setStyleSheet("color:#506070;font-size:9px;background:transparent;border:none;");
    auto *xBtn=new QPushButton("\xE2\x9C\x95");
    xBtn->setFixedSize(20,20);
    xBtn->setStyleSheet("QPushButton{background:transparent;border:none;color:#506070;font-size:13px;}QPushButton:hover{color:#ff4444;}");
    connect(xBtn,&QPushButton::clicked,this,&KeymapSidePanel::closeOverlay);
    tr->addWidget(tl); tr->addStretch(); tr->addWidget(xBtn);
    root->addLayout(tr); root->addWidget(m_fileLabel);

    auto mkSep=[&](){
        auto *s=new QFrame(); s->setFrameShape(QFrame::HLine);
        s->setStyleSheet("border:none;border-top:1px solid #1e3a58;background:transparent;");
        return s;
    };
    root->addWidget(mkSep());

    // Add group
    auto *ag=new QGroupBox("Add Controls");
    auto *al=new QVBoxLayout(ag); al->setContentsMargins(4,4,4,4); al->setSpacing(3);
    auto *r1=new QHBoxLayout();
    auto *clickBtn=new QPushButton("+ Click");
    auto *dblBtn  =new QPushButton("+ Double");
    auto *joyBtn  =new QPushButton("\xE2\x8C\x82 + WASD Joy");
    auto *aimBtn  =new QPushButton("\xF0\x9F\x8E\xAF + Mouse Aim");
    auto *eyeBtn  =new QPushButton("\xF0\x9F\x91\x81 + Free Look");
    connect(clickBtn,&QPushButton::clicked,this,&KeymapSidePanel::addClick);
    connect(dblBtn,  &QPushButton::clicked,this,&KeymapSidePanel::addClickTwice);
    connect(joyBtn,  &QPushButton::clicked,this,&KeymapSidePanel::addJoystick);
    connect(aimBtn,  &QPushButton::clicked,this,&KeymapSidePanel::addMouseAim);
    connect(eyeBtn,  &QPushButton::clicked,this,&KeymapSidePanel::addSmallEyes);
    r1->addWidget(clickBtn); r1->addWidget(dblBtn);
    al->addLayout(r1); al->addWidget(joyBtn); al->addWidget(aimBtn); al->addWidget(eyeBtn);
    root->addWidget(ag);

    // Props scroll
    m_propsScroll=new QScrollArea(); m_propsScroll->setWidgetResizable(true);
    m_propsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *pc=new QWidget(); pc->setStyleSheet("background:transparent;");
    auto *pv=new QVBoxLayout(pc); pv->setContentsMargins(0,0,0,0); pv->setSpacing(3);
    m_propsGroup=new QGroupBox("Properties");
    auto *pi=new QVBoxLayout(m_propsGroup); pi->setContentsMargins(4,4,4,4); pi->setSpacing(3);

    pi->addWidget(new QLabel("Label:"));
    m_commentEdit=new QLineEdit(); pi->addWidget(m_commentEdit);
    pi->addWidget(new QLabel("Type:"));
    m_typeCombo=new QComboBox();
    m_typeCombo->addItems({"Click","Double Click","WASD Joystick","Mouse Aim","Free Look"});
    pi->addWidget(m_typeCombo);

    // Click widget
    m_clickWidget=new QWidget();
    auto *cl=new QVBoxLayout(m_clickWidget); cl->setContentsMargins(0,0,0,0); cl->setSpacing(2);
    cl->addWidget(new QLabel("Key:")); m_keyBtn=new KeyRecordButton(); cl->addWidget(m_keyBtn);
    m_switchChk=new QCheckBox("Toggle keymap on press"); cl->addWidget(m_switchChk);
    pi->addWidget(m_clickWidget);

    // Joy widget
    m_joyWidget=new QWidget();
    auto *jl=new QVBoxLayout(m_joyWidget); jl->setContentsMargins(0,0,0,0); jl->setSpacing(2);
    m_upBtn=new KeyRecordButton(); m_downBtn=new KeyRecordButton();
    m_leftBtn=new KeyRecordButton(); m_rightBtn=new KeyRecordButton();
    auto *jr1=new QHBoxLayout(); jr1->addWidget(new QLabel("Up:"));   jr1->addWidget(m_upBtn);
    auto *jr2=new QHBoxLayout(); jr2->addWidget(new QLabel("Down:")); jr2->addWidget(m_downBtn);
    auto *jr3=new QHBoxLayout(); jr3->addWidget(new QLabel("L:"));    jr3->addWidget(m_leftBtn);
                                 jr3->addWidget(new QLabel("R:"));    jr3->addWidget(m_rightBtn);
    jl->addLayout(jr1); jl->addLayout(jr2); jl->addLayout(jr3);
    jl->addWidget(new QLabel("Radius:"));
    m_offsetSpin=new QDoubleSpinBox(); m_offsetSpin->setRange(0.02,0.3);
    m_offsetSpin->setSingleStep(0.01); m_offsetSpin->setDecimals(2);
    jl->addWidget(m_offsetSpin); pi->addWidget(m_joyWidget);

    // Aim widget
    m_aimWidget=new QWidget();
    auto *aiml=new QVBoxLayout(m_aimWidget); aiml->setContentsMargins(0,0,0,0); aiml->setSpacing(2);
    m_speedXSpin=new QDoubleSpinBox(); m_speedXSpin->setRange(0.1,20); m_speedXSpin->setValue(3.0); m_speedXSpin->setSingleStep(0.1);
    m_speedYSpin=new QDoubleSpinBox(); m_speedYSpin->setRange(0.1,20); m_speedYSpin->setValue(1.5); m_speedYSpin->setSingleStep(0.1);
    auto *sx=new QHBoxLayout(); sx->addWidget(new QLabel("Speed X:")); sx->addWidget(m_speedXSpin);
    auto *sy=new QHBoxLayout(); sy->addWidget(new QLabel("Speed Y:")); sy->addWidget(m_speedYSpin);
    aiml->addLayout(sx); aiml->addLayout(sy); pi->addWidget(m_aimWidget);

    // Eye widget
    m_eyeWidget=new QWidget();
    auto *el=new QVBoxLayout(m_eyeWidget); el->setContentsMargins(0,0,0,0); el->setSpacing(2);
    el->addWidget(new QLabel("Toggle Key:"));
    m_eyeKeyBtn=new KeyRecordButton(); el->addWidget(m_eyeKeyBtn);
    pi->addWidget(m_eyeWidget);

    // Action buttons
    auto *ab=new QHBoxLayout();
    m_applyBtn=new QPushButton("\xE2\x9C\x93 Apply");
    m_applyBtn->setStyleSheet("QPushButton{background:#0a3a1a;color:#40ff80;border:1px solid #0a8040;border-radius:5px;padding:4px;}"
                              "QPushButton:hover{background:#0a5a28;}");
    m_deleteBtn=new QPushButton("\xE2\x9C\x95 Del");
    m_deleteBtn->setStyleSheet("QPushButton{background:#3a0a0a;color:#ff6060;border:1px solid #800a0a;border-radius:5px;padding:4px;}"
                               "QPushButton:hover{background:#5a1010;}");
    m_dupBtn=new QPushButton("Dup");
    connect(m_applyBtn, &QPushButton::clicked,this,&KeymapSidePanel::applyProps);
    connect(m_deleteBtn,&QPushButton::clicked,this,&KeymapSidePanel::deleteSelected);
    connect(m_dupBtn,   &QPushButton::clicked,this,&KeymapSidePanel::duplicateSelected);
    ab->addWidget(m_applyBtn); ab->addWidget(m_dupBtn); ab->addWidget(m_deleteBtn);
    pi->addLayout(ab);

    pv->addWidget(m_propsGroup); pv->addStretch();
    m_propsScroll->setWidget(pc);
    root->addWidget(m_propsScroll,1);

    root->addWidget(mkSep());

    // Save & apply
    auto *sab=new QPushButton("\xF0\x9F\x92\xBE Save & Apply to Device");
    sab->setStyleSheet("QPushButton{background:#0a3060;color:#60c8ff;border:1px solid #1060c0;border-radius:6px;padding:6px;font-size:12px;font-weight:bold;}"
                       "QPushButton:hover{background:#1050a0;}");
    connect(sab,&QPushButton::clicked,this,&KeymapSidePanel::saveAndApply);
    root->addWidget(sab);

    auto *bot=new QHBoxLayout();
    auto *newBtn=new QPushButton("New"); auto *saBtn=new QPushButton("Save As"); auto *impBtn=new QPushButton("Import");
    connect(newBtn,&QPushButton::clicked,this,&KeymapSidePanel::newLayout);
    connect(saBtn, &QPushButton::clicked,this,&KeymapSidePanel::saveAs);
    connect(impBtn,&QPushButton::clicked,this,&KeymapSidePanel::importFile);
    bot->addWidget(newBtn); bot->addWidget(saBtn); bot->addWidget(impBtn);
    root->addLayout(bot);

    clearProps();
}

void KeymapSidePanel::setCurrentFile(const QString &f) {
    m_currentFile=f;
    m_fileLabel->setText(f.isEmpty()?"(New Layout)":QFileInfo(f).fileName());
}

void KeymapSidePanel::clearProps() {
    m_propsGroup->setVisible(false);
    m_clickWidget->setVisible(false); m_joyWidget->setVisible(false);
    m_aimWidget->setVisible(false);   m_eyeWidget->setVisible(false);
    m_applyBtn->setEnabled(false); m_deleteBtn->setEnabled(false); m_dupBtn->setEnabled(false);
}

void KeymapSidePanel::loadNode(int idx, QList<KeyNode> *nodes,
                               bool hasMouseMove, QPointF /*mousePos*/,
                               double speedX, double speedY,
                               bool /*hasSmallEyes*/, const QString &eyeKey, QPointF /*eyePos*/) {
    clearProps();
    if (idx==-1 || !nodes) return;
    m_propsGroup->setVisible(true); m_applyBtn->setEnabled(true);
    m_deleteBtn->setEnabled(true);  m_dupBtn->setEnabled(idx>=0);
    if (idx==-2) {
        m_propsGroup->setTitle("Mouse Aim"); m_commentEdit->setText("Aim Look");
        m_typeCombo->setCurrentIndex(3);
        m_speedXSpin->setValue(speedX); m_speedYSpin->setValue(speedY);
        m_aimWidget->setVisible(true);
    } else if (idx==-3) {
        m_propsGroup->setTitle("Free Look"); m_commentEdit->setText("Free Look");
        m_typeCombo->setCurrentIndex(4);
        m_eyeKeyBtn->setRecordedKey(eyeKey); m_eyeWidget->setVisible(true);
    } else if (idx>=0 && idx<nodes->size()) {
        const KeyNode &n=nodes->at(idx);
        m_commentEdit->setText(n.comment);
        if (n.type==KeyNode::SteerWheel) {
            m_propsGroup->setTitle("WASD Joystick"); m_typeCombo->setCurrentIndex(2);
            m_upBtn->setRecordedKey(n.upKey); m_downBtn->setRecordedKey(n.downKey);
            m_leftBtn->setRecordedKey(n.leftKey); m_rightBtn->setRecordedKey(n.rightKey);
            m_offsetSpin->setValue(n.upOffset); m_joyWidget->setVisible(true);
        } else {
            m_propsGroup->setTitle(n.type==KeyNode::ClickTwice?"Double Click":"Click Key");
            m_typeCombo->setCurrentIndex(n.type==KeyNode::ClickTwice?1:0);
            m_keyBtn->setRecordedKey(n.key); m_switchChk->setChecked(n.switchMap);
            m_clickWidget->setVisible(true);
        }
    }
}

void KeymapSidePanel::saveNodeProps(int idx, QList<KeyNode> *nodes) {
    if (!nodes||idx<0||idx>=nodes->size()) return;
    KeyNode &n=(*nodes)[idx]; n.comment=m_commentEdit->text();
    if (n.type==KeyNode::SteerWheel) {
        n.upKey=m_upBtn->recordedKey(); n.downKey=m_downBtn->recordedKey();
        n.leftKey=m_leftBtn->recordedKey(); n.rightKey=m_rightBtn->recordedKey();
        double off=m_offsetSpin->value();
        n.leftOffset=n.rightOffset=n.upOffset=n.downOffset=off;
    } else { n.key=m_keyBtn->recordedKey(); n.switchMap=m_switchChk->isChecked(); }
}

void KeymapSidePanel::readMouseAimProps(bool &has, QPointF &/*pos*/, double &sx, double &sy) {
    has=true; sx=m_speedXSpin->value(); sy=m_speedYSpin->value();
}

void KeymapSidePanel::readSmallEyesProps(bool &has, QString &key, QPointF &/*pos*/) {
    has=true; key=m_eyeKeyBtn->recordedKey();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  KeymapEditorController
// ═══════════════════════════════════════════════════════════════════════════════
KeymapEditorController::KeymapEditorController(const QString &serial,
                                               QWidget *videoContainer,
                                               QWidget *sideParent,
                                               QObject *parent)
    : QObject(parent), m_serial(serial),
      m_videoContainer(videoContainer), m_sideParent(sideParent)
{
    m_overlay=new KeymapOverlay(m_videoContainer);
    m_overlay->setNodes(&m_nodes);
    m_overlay->hide();

    m_panel=new KeymapSidePanel(nullptr);
    m_panel->hide();

    connect(m_overlay,&KeymapOverlay::nodeSelected,        this,&KeymapEditorController::onNodeSelected);
    connect(m_overlay,&KeymapOverlay::nodeMoved,           this,&KeymapEditorController::onNodeMoved);
    connect(m_overlay,&KeymapOverlay::mouseAimMoved,       this,&KeymapEditorController::onMouseAimMoved);
    connect(m_overlay,&KeymapOverlay::smallEyesMoved,      this,&KeymapEditorController::onSmallEyesMoved);
    connect(m_overlay,&KeymapOverlay::overlayDoubleClicked,this,&KeymapEditorController::onOverlayDoubleClicked);
    connect(m_overlay,&KeymapOverlay::requestDelete,       this,&KeymapEditorController::onRequestDelete);

    connect(m_panel,&KeymapSidePanel::addClick,         this,&KeymapEditorController::onAddClick);
    connect(m_panel,&KeymapSidePanel::addClickTwice,    this,&KeymapEditorController::onAddClickTwice);
    connect(m_panel,&KeymapSidePanel::addJoystick,      this,&KeymapEditorController::onAddJoystick);
    connect(m_panel,&KeymapSidePanel::addMouseAim,      this,&KeymapEditorController::onAddMouseAim);
    connect(m_panel,&KeymapSidePanel::addSmallEyes,     this,&KeymapEditorController::onAddSmallEyes);
    connect(m_panel,&KeymapSidePanel::applyProps,       this,&KeymapEditorController::onApplyProps);
    connect(m_panel,&KeymapSidePanel::deleteSelected,   this,&KeymapEditorController::onDeleteSelected);
    connect(m_panel,&KeymapSidePanel::duplicateSelected,this,&KeymapEditorController::onDuplicateSelected);
    connect(m_panel,&KeymapSidePanel::saveAndApply,     this,&KeymapEditorController::onSaveAndApply);
    connect(m_panel,&KeymapSidePanel::saveAs,           this,&KeymapEditorController::onSaveAs);
    connect(m_panel,&KeymapSidePanel::importFile,       this,&KeymapEditorController::onImportFile);
    connect(m_panel,&KeymapSidePanel::newLayout,        this,&KeymapEditorController::onNewLayout);
    connect(m_panel,&KeymapSidePanel::closeOverlay,     this,&KeymapEditorController::onClose);
}

KeymapEditorController::~KeymapEditorController() {
    if (m_panel)   { m_panel->hide();   delete m_panel;   m_panel=nullptr; }
    // overlay owned by videoContainer widget, no need to delete
}

bool KeymapEditorController::eventFilter(QObject *obj, QEvent *ev) {
    if (obj==m_videoContainer && ev->type()==QEvent::Resize && m_overlay->isVisible()) {
        m_overlay->setGeometry(m_videoContainer->rect());
        QPoint gp=m_videoContainer->mapToGlobal(QPoint(m_videoContainer->width()+4,0));
        m_panel->move(gp);
    }
    return QObject::eventFilter(obj,ev);
}

void KeymapEditorController::show() {
    if (!m_videoContainer) return;
    m_overlay->setGeometry(m_videoContainer->rect());
    m_overlay->raise(); m_overlay->show();
    QPoint gp=m_videoContainer->mapToGlobal(QPoint(m_videoContainer->width()+4,0));
    m_panel->move(gp);
    m_panel->setMinimumHeight(qMin(580,m_videoContainer->height()));
    m_panel->show(); m_panel->raise();
    m_videoContainer->installEventFilter(this);
    syncOverlay(); m_panel->clearProps(); m_panel->setCurrentFile(m_currentFile);
}

void KeymapEditorController::hide() {
    m_overlay->hide(); m_panel->hide();
    if (m_videoContainer) m_videoContainer->removeEventFilter(this);
}

bool KeymapEditorController::isVisible() const { return m_overlay&&m_overlay->isVisible(); }

void KeymapEditorController::syncOverlay() {
    m_overlay->setNodes(&m_nodes);
    m_overlay->setMouseMoveMap(m_hasMouseMove,m_mouseStartPos,m_mouseSpeedX,m_mouseSpeedY,
                               m_smallEyesKey,m_smallEyesPos);
    m_overlay->update();
}

void KeymapEditorController::onNodeSelected(int idx) {
    m_selIdx=idx;
    m_panel->loadNode(idx,&m_nodes,m_hasMouseMove,m_mouseStartPos,
                      m_mouseSpeedX,m_mouseSpeedY,m_hasSmallEyes,m_smallEyesKey,m_smallEyesPos);
}
void KeymapEditorController::onNodeMoved(int,QPointF){}
void KeymapEditorController::onMouseAimMoved(QPointF r){m_mouseStartPos=r;}
void KeymapEditorController::onSmallEyesMoved(QPointF r){m_smallEyesPos=r;}

void KeymapEditorController::onOverlayDoubleClicked(QPointF r) {
    KeyNode n; n.type=KeyNode::Click; n.key="Key_Space"; n.pos=r; n.comment="Key";
    m_nodes.append(n); m_selIdx=m_nodes.size()-1;
    m_overlay->setSelectedIndex(m_selIdx); syncOverlay(); onNodeSelected(m_selIdx);
}

void KeymapEditorController::onRequestDelete(int idx) {
    if (idx==-999) { onDuplicateSelected(); return; }
    m_selIdx=idx; onDeleteSelected();
}

void KeymapEditorController::onAddClick() {
    KeyNode n; n.type=KeyNode::Click; n.pos=QPointF(0.5,0.5); n.comment="Key";
    m_nodes.append(n); m_selIdx=m_nodes.size()-1;
    m_overlay->setSelectedIndex(m_selIdx); syncOverlay(); onNodeSelected(m_selIdx);
}
void KeymapEditorController::onAddClickTwice() {
    KeyNode n; n.type=KeyNode::ClickTwice; n.key="Key_E"; n.pos=QPointF(0.6,0.5); n.comment="Double";
    m_nodes.append(n); m_selIdx=m_nodes.size()-1;
    m_overlay->setSelectedIndex(m_selIdx); syncOverlay(); onNodeSelected(m_selIdx);
}
void KeymapEditorController::onAddJoystick() {
    KeyNode n; n.type=KeyNode::SteerWheel; n.comment="WASD"; n.centerPos=QPointF(0.18,0.72);
    m_nodes.append(n); m_selIdx=m_nodes.size()-1;
    m_overlay->setSelectedIndex(m_selIdx); syncOverlay(); onNodeSelected(m_selIdx);
}
void KeymapEditorController::onAddMouseAim()  { m_hasMouseMove=true;  m_selIdx=-2; m_overlay->setSelectedIndex(-2); syncOverlay(); onNodeSelected(-2); }
void KeymapEditorController::onAddSmallEyes() { m_hasSmallEyes=true;  m_selIdx=-3; m_overlay->setSelectedIndex(-3); syncOverlay(); onNodeSelected(-3); }

void KeymapEditorController::onApplyProps() {
    if (m_selIdx>=0&&m_selIdx<m_nodes.size()) m_panel->saveNodeProps(m_selIdx,&m_nodes);
    else if (m_selIdx==-2) m_panel->readMouseAimProps(m_hasMouseMove,m_mouseStartPos,m_mouseSpeedX,m_mouseSpeedY);
    else if (m_selIdx==-3) m_panel->readSmallEyesProps(m_hasSmallEyes,m_smallEyesKey,m_smallEyesPos);
    syncOverlay();
}

void KeymapEditorController::onDeleteSelected() {
    if      (m_selIdx==-2) m_hasMouseMove=false;
    else if (m_selIdx==-3) m_hasSmallEyes=false;
    else if (m_selIdx>=0&&m_selIdx<m_nodes.size()) m_nodes.removeAt(m_selIdx);
    m_selIdx=-1; m_overlay->setSelectedIndex(-1); m_panel->clearProps(); syncOverlay();
}

void KeymapEditorController::onDuplicateSelected() {
    if (m_selIdx>=0&&m_selIdx<m_nodes.size()) {
        KeyNode c=m_nodes.at(m_selIdx);
        c.pos+=QPointF(0.04,0.04); c.centerPos+=QPointF(0.04,0.04); c.comment+=" (Copy)";
        m_nodes.append(c); m_selIdx=m_nodes.size()-1;
        m_overlay->setSelectedIndex(m_selIdx); syncOverlay(); onNodeSelected(m_selIdx);
    }
}

void KeymapEditorController::onSaveAndApply() {
    onApplyProps();
    QString path=currentFilePath();
    if (saveJson(path)) { applyToDevice(); QMessageBox::information(m_panel,"Saved","Keymap saved and applied to device!"); }
    else                QMessageBox::warning(m_panel,"Save Failed","Could not write:\n"+path);
}

void KeymapEditorController::onSaveAs() {
    bool ok; QString name=QInputDialog::getText(m_panel,"Save As","Profile name:",QLineEdit::Normal,"MyGame",&ok);
    if (!ok||name.trimmed().isEmpty()) return;
    QString path=userKeymapDir()+"/"+name.trimmed()+".json";
    if (saveJson(path)) { m_currentFile=path; m_panel->setCurrentFile(path); applyToDevice(); }
}

void KeymapEditorController::onImportFile() {
    QString p=QFileDialog::getOpenFileName(m_panel,"Import Keymap",defaultKeymapDir(),"JSON (*.json)");
    if (!p.isEmpty()) loadJson(p);
}

void KeymapEditorController::onNewLayout() {
    m_nodes.clear(); m_hasMouseMove=true; m_hasSmallEyes=false;
    m_mouseStartPos=QPointF(0.55,0.5); m_selIdx=-1; m_currentFile.clear();
    KeyNode w; w.type=KeyNode::SteerWheel; w.comment="WASD"; w.centerPos=QPointF(0.18,0.72); m_nodes.append(w);
    m_panel->setCurrentFile(QString()); m_panel->clearProps(); syncOverlay();
}

void KeymapEditorController::onClose() { hide(); }

QString KeymapEditorController::defaultKeymapDir() const { return QApplication::applicationDirPath()+"/keymap"; }
QString KeymapEditorController::userKeymapDir()    const {
    QString d=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/keymap";
    QDir().mkpath(d); return d;
}
QString KeymapEditorController::currentFilePath()  const {
    return m_currentFile.isEmpty()?(userKeymapDir()+"/default.json"):m_currentFile;
}

bool KeymapEditorController::loadJson(const QString &path) {
    QFile f(path); if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc=QJsonDocument::fromJson(f.readAll()); f.close();
    QJsonObject root=doc.object();
    m_switchKey=root["switchKey"].toString("Key_QuoteLeft");
    if (root.contains("mouseMoveMap")) {
        m_hasMouseMove=true;
        QJsonObject mm=root["mouseMoveMap"].toObject(), sp=mm["startPos"].toObject();
        m_mouseStartPos=QPointF(sp["x"].toDouble(0.55),sp["y"].toDouble(0.5));
        m_mouseSpeedX=mm["speedRatioX"].toDouble(3.0); m_mouseSpeedY=mm["speedRatioY"].toDouble(1.5);
        if (mm.contains("smallEyes")) {
            m_hasSmallEyes=true;
            QJsonObject se=mm["smallEyes"].toObject(), sep=se["pos"].toObject();
            m_smallEyesKey=se["key"].toString("Key_Alt");
            m_smallEyesPos=QPointF(sep["x"].toDouble(0.8),sep["y"].toDouble(0.3));
        } else m_hasSmallEyes=false;
    } else { m_hasMouseMove=false; m_hasSmallEyes=false; }
    m_nodes.clear();
    for (const QJsonValue &v:root["keyMapNodes"].toArray()) m_nodes.append(KeyNode::fromJson(v.toObject()));
    m_currentFile=path; m_panel->setCurrentFile(path);
    m_selIdx=-1; m_overlay->setSelectedIndex(-1); m_panel->clearProps();
    syncOverlay(); return true;
}

bool KeymapEditorController::saveJson(const QString &path) {
    QJsonObject root; root["switchKey"]=m_switchKey;
    if (m_hasMouseMove) {
        QJsonObject mm,sp; sp["x"]=m_mouseStartPos.x(); sp["y"]=m_mouseStartPos.y();
        mm["startPos"]=sp; mm["speedRatioX"]=m_mouseSpeedX; mm["speedRatioY"]=m_mouseSpeedY; mm["speedRatio"]=10;
        if (m_hasSmallEyes) {
            QJsonObject se,sep; se["comment"]="Free Look"; se["type"]="KMT_CLICK"; se["key"]=m_smallEyesKey;
            sep["x"]=m_smallEyesPos.x(); sep["y"]=m_smallEyesPos.y(); se["pos"]=sep; se["switchMap"]=false;
            mm["smallEyes"]=se;
        }
        root["mouseMoveMap"]=mm;
    }
    QJsonArray arr; for (const KeyNode &n:m_nodes) arr.append(n.toJson());
    root["keyMapNodes"]=arr;
    QFile f(path); if (!f.open(QIODevice::WriteOnly|QIODevice::Truncate)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented)); f.close();
    m_currentFile=path; m_panel->setCurrentFile(path); return true;
}

void KeymapEditorController::applyToDevice() {
    auto device=qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) return;
    QFile f(currentFilePath()); if (!f.open(QIODevice::ReadOnly)) return;
    device->updateScript(QString::fromUtf8(f.readAll())); f.close();
}
