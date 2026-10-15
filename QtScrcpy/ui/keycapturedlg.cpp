#include "keycapturedlg.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMetaEnum>

KeyCaptureDlg::KeyCaptureDlg(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Press a Key or Mouse Button"));
    setModal(true);
    setFixedSize(340, 180);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // --- layout ---
    auto *vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(20, 20, 20, 20);
    vlay->setSpacing(12);

    m_hint = new QLabel(tr("Press any keyboard key or mouse button…"), this);
    m_hint->setAlignment(Qt::AlignCenter);
    m_hint->setWordWrap(true);
    vlay->addWidget(m_hint);

    m_result = new QLabel(tr("—"), this);
    m_result->setAlignment(Qt::AlignCenter);
    QFont f = m_result->font();
    f.setPointSize(14);
    f.setBold(true);
    m_result->setFont(f);
    m_result->setStyleSheet("color: #00e0ff;");
    vlay->addWidget(m_result);

    auto *btnRow = new QHBoxLayout();
    m_accept = new QPushButton(tr("OK"), this);
    m_cancel = new QPushButton(tr("Cancel"), this);
    m_accept->setEnabled(false);
    btnRow->addStretch();
    btnRow->addWidget(m_accept);
    btnRow->addWidget(m_cancel);
    vlay->addLayout(btnRow);

    connect(m_accept, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancel, &QPushButton::clicked, this, &QDialog::reject);

    // Grab keyboard focus so key events arrive here
    setFocusPolicy(Qt::StrongFocus);
}

void KeyCaptureDlg::setCaptured(const QString &key)
{
    m_captured = key;
    m_result->setText(key);
    m_accept->setEnabled(true);
}

void KeyCaptureDlg::keyPressEvent(QKeyEvent *event)
{
    // Ignore modifier-only presses, Escape (=cancel)
    int key = event->key();
    if (key == Qt::Key_Escape) {
        reject();
        return;
    }
    if (key == Qt::Key_Control || key == Qt::Key_Shift ||
        key == Qt::Key_Alt    || key == Qt::Key_Meta) {
        return; // wait for a real key
    }

    // Build the Qt key name via QMetaEnum
    QMetaEnum me = QMetaEnum::fromType<Qt::Key>();
    const char *keyName = me.valueToKey(key);
    QString name = keyName ? QString::fromLatin1(keyName) : QStringLiteral("Key_%1").arg(key);

    setCaptured(name);
    event->accept();
}

void KeyCaptureDlg::mousePressEvent(QMouseEvent *event)
{
    Qt::MouseButton btn = event->button();
    QString name;
    switch (btn) {
    case Qt::LeftButton:   name = QStringLiteral("LeftButton");   break;
    case Qt::RightButton:  name = QStringLiteral("RightButton");  break;
    case Qt::MiddleButton: name = QStringLiteral("MiddleButton"); break;
    default:               name = QStringLiteral("MouseButton%1").arg(static_cast<int>(btn)); break;
    }
    setCaptured(name);
    event->accept();
}
