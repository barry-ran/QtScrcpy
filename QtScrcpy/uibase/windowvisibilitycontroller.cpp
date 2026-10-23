#include "windowvisibilitycontroller.h"

#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QGuiApplication>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QWidget>
#include <QWindow>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <QHash>
#endif

namespace {
constexpr int VisibilityDelayMs = 300;
constexpr int OutsideMargin = 12;
constexpr qreal HiddenOpacity = 1.0 / 255.0;
}

struct WindowVisibilityController::NativeState
{
    explicit NativeState(WindowVisibilityController *controller) : owner(controller) {}
    ~NativeState() { stop(); }

    WindowVisibilityController *owner;

#ifdef Q_OS_WIN
    struct ImeCandidate {
        HWND window = nullptr;
        HWND focus = nullptr;
        DWORD thread = 0;
        DWORD process = 0;
        LONG object = 0;
        LONG child = 0;
        bool compatibility = false;
    };

    HWINEVENTHOOK imeHook = nullptr;
    HWINEVENTHOOK windowHook = nullptr;
    QList<ImeCandidate> candidates;

    static QHash<HWINEVENTHOOK, NativeState *> &hooks()
    {
        static QHash<HWINEVENTHOOK, NativeState *> subscriptions;
        return subscriptions;
    }

    bool belongs(HWND target) const
    {
        if (!target)
            return false;
        target = GetAncestor(target, GA_ROOT);
        for (int depth = 0; target && depth < 32; ++depth) {
            for (QWidget *window : owner->groupWindows()) {
                if (window->internalWinId() && reinterpret_cast<HWND>(window->internalWinId()) == target)
                    return true;
            }
            target = GetWindow(target, GW_OWNER);
        }
        return false;
    }

    static HWND focusedWindow()
    {
        GUITHREADINFO gui = {};
        gui.cbSize = sizeof(gui);
        return GetGUIThreadInfo(0, &gui) ? gui.hwndFocus : nullptr;
    }

    // WeType can report ordinary window visibility events instead of IME
    // events. Only its nonactivating candidate-window shape is exempted.
    static bool isCompatibilityCandidate(HWND window)
    {
        wchar_t name[64] = {};
        RECT rect = {};
        const LONG_PTR required = WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
        return IsWindowVisible(window)
            && (GetWindowLongPtr(window, GWL_EXSTYLE) & required) == required
            && GetClassNameW(window, name, 64)
            && !lstrcmpW(name, L"wetype.flutter.setting")
            && GetWindowRect(window, &rect) && rect.right > rect.left && rect.bottom > rect.top;
    }

    void pruneCandidates()
    {
        const HWND focus = focusedWindow();
        const bool groupFocused = belongs(focus) && belongs(GetForegroundWindow());
        for (int i = candidates.size() - 1; i >= 0; --i) {
            const ImeCandidate &candidate = candidates.at(i);
            DWORD process = 0;
            if (!groupFocused || candidate.focus != focus || !IsWindowVisible(candidate.window)
                || GetWindowThreadProcessId(candidate.window, &process) != candidate.thread
                || process != candidate.process
                || (candidate.compatibility && !isCompatibilityCandidate(candidate.window))) {
                candidates.removeAt(i);
            }
        }
    }

    static void CALLBACK imeEvent(HWINEVENTHOOK hook, DWORD event, HWND window,
                                  LONG object, LONG child, DWORD, DWORD)
    {
        NativeState *state = hooks().value(hook, nullptr);
        if (!state || !window)
            return;
        const bool compatibility = hook == state->windowHook;
        const bool hiding = event == EVENT_OBJECT_HIDE || event == 0x8028; // EVENT_OBJECT_IME_HIDE
        if (compatibility) {
            if (object != OBJID_WINDOW || child != CHILDID_SELF
                || (event != EVENT_OBJECT_SHOW && event != EVENT_OBJECT_HIDE)
                || (!hiding && !isCompatibilityCandidate(window)))
                return;
        } else if (event < 0x8027 || event > 0x8029) { // IME_SHOW, IME_HIDE, IME_CHANGE
            return;
        }
        state->pruneCandidates();
        for (int i = state->candidates.size() - 1; i >= 0; --i) {
            ImeCandidate &candidate = state->candidates[i];
            if (candidate.window == window && ((compatibility && hiding)
                || (candidate.object == object && candidate.child == child))) {
                if (hiding)
                    state->candidates.removeAt(i);
                else if (!compatibility)
                    candidate.compatibility = false;
                if (!(compatibility && hiding))
                    return;
            }
        }
        const HWND focus = focusedWindow();
        if (hiding || !state->belongs(focus) || !state->belongs(GetForegroundWindow()) || !IsWindowVisible(window))
            return;
        ImeCandidate candidate;
        candidate.window = window;
        candidate.focus = focus;
        candidate.thread = GetWindowThreadProcessId(window, &candidate.process);
        candidate.object = object;
        candidate.child = child;
        candidate.compatibility = compatibility;
        if (candidate.thread)
            state->candidates.append(candidate);
    }

    void start()
    {
        if (!imeHook) {
            imeHook = SetWinEventHook(0x8027, 0x8029, nullptr, imeEvent, 0, 0,
                                     WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
            if (imeHook)
                hooks().insert(imeHook, this);
        }
        if (!windowHook) {
            windowHook = SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, nullptr, imeEvent, 0, 0,
                                        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
            if (windowHook)
                hooks().insert(windowHook, this);
        }
    }

    void stop()
    {
        if (imeHook) {
            hooks().remove(imeHook);
            UnhookWinEvent(imeHook);
            imeHook = nullptr;
        }
        if (windowHook) {
            hooks().remove(windowHook);
            UnhookWinEvent(windowHook);
            windowHook = nullptr;
        }
        candidates.clear();
    }

    bool busy()
    {
        pruneCandidates();
        if (!candidates.isEmpty())
            return true;
        if (!owner->m_host || !owner->m_host->internalWinId())
            return false;
        const DWORD thread = GetWindowThreadProcessId(reinterpret_cast<HWND>(owner->m_host->internalWinId()), nullptr);
        GUITHREADINFO gui = {};
        gui.cbSize = sizeof(gui);
        if (!thread || !GetGUIThreadInfo(thread, &gui))
            return false;
        return ((gui.flags & GUI_INMOVESIZE) && belongs(gui.hwndMoveSize))
            || ((gui.flags & (GUI_INMENUMODE | GUI_SYSTEMMENUMODE | GUI_POPUPMENUMODE)) && belongs(gui.hwndMenuOwner))
            || belongs(gui.hwndCapture);
    }
#else
    void start() {}
    void stop() {}
    bool busy() { return false; }
#endif
};

WindowVisibilityController::WindowVisibilityController(QWidget *host)
    : QObject(host), m_host(host), m_native(new NativeState(this))
{
    m_clock.start();
    m_timer.setInterval(50);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &WindowVisibilityController::tick);
    addWindow(host);
    if (qApp)
        qApp->installEventFilter(this);
}

WindowVisibilityController::~WindowVisibilityController()
{
    m_timer.stop();
    m_native->stop();
    if (qApp)
        qApp->removeEventFilter(this);
    // An independently destroyed controller must not leave invisible windows.
    for (const QPointer<QWidget> &window : m_appliedWindows) {
        if (window)
            window->setWindowOpacity(1.0);
    }
}

void WindowVisibilityController::addWindow(QWidget *window)
{
    if (!window)
        return;
    window = window->window();
    if (!m_windows.contains(window))
        m_windows.append(window);
    applyOpacity(window);
}

void WindowVisibilityController::setOpacityPercent(int percent)
{
    percent = qBound(10, percent, 100);
    if (m_opacityPercent == percent)
        return;
    m_opacityPercent = percent;
    applyOpacity();
    emit opacityPercentChanged(percent);
}

int WindowVisibilityController::opacityPercent() const
{
    return m_opacityPercent;
}

void WindowVisibilityController::setAutoHideEnabled(bool enabled)
{
    if (m_autoHideEnabled == enabled)
        return;
    m_autoHideEnabled = enabled;
    resetVisibility();
    if (enabled) {
        m_native->start();
        m_timer.start();
    } else {
        m_timer.stop();
        m_native->stop();
    }
    emit autoHideEnabledChanged(enabled);
}

bool WindowVisibilityController::autoHideEnabled() const
{
    return m_autoHideEnabled;
}

bool WindowVisibilityController::belongsToGroup(QWidget *widget) const
{
    if (!widget)
        return false;
    QWidget *window = widget->window();
    for (QWidget *ancestor = widget; ancestor; ancestor = ancestor->parentWidget()) {
        if (m_windows.contains(ancestor) || m_windows.contains(ancestor->window()))
            return true;
    }
    // Some Qt popups are transient QWindows without QWidget ownership.
    for (QWindow *transient = window->windowHandle(); transient; transient = transient->transientParent()) {
        for (const QPointer<QWidget> &managed : m_windows) {
            if (managed && managed->windowHandle() == transient)
                return true;
        }
    }
    return false;
}

QList<QWidget *> WindowVisibilityController::groupWindows() const
{
    QList<QWidget *> result;
    if (qApp) {
        for (QWidget *window : QApplication::topLevelWidgets()) {
            if (belongsToGroup(window))
                result.append(window);
        }
    }
    for (const QPointer<QWidget> &window : m_windows) {
        if (window && !result.contains(window))
            result.append(window);
    }
    return result;
}

bool WindowVisibilityController::pointerIsNear(bool includeMargin) const
{
#ifdef Q_OS_WIN
    POINT cursor = {};
    if (!GetCursorPos(&cursor))
        return true; // Missing pointer information must never strand the UI.
    if (m_native->belongs(WindowFromPoint(cursor)))
        return true;
    if (!includeMargin)
        return false;
    bool outsideNear = false;
    for (QWidget *window : groupWindows()) {
        if (!window->isVisible() || window->isMinimized() || !window->internalWinId())
            continue;
        RECT rect = {};
        if (!GetWindowRect(reinterpret_cast<HWND>(window->internalWinId()), &rect))
            continue;
        // Native pixels throughout; scale only the logical outside tolerance.
        // An overlapping window inside the frame never counts as hovering.
        if (PtInRect(&rect, cursor))
            return false;
        const int margin = qRound(OutsideMargin * window->devicePixelRatioF());
        InflateRect(&rect, margin, margin);
        outsideNear = outsideNear || PtInRect(&rect, cursor);
    }
    return outsideNear;
#else
    const QPoint cursor = QCursor::pos();
    if (belongsToGroup(QApplication::widgetAt(cursor)))
        return true;
    if (!includeMargin)
        return false;
    bool outsideNear = false;
    for (QWidget *window : groupWindows()) {
        if (!window->isVisible() || window->isMinimized())
            continue;
        const QRect frame = window->frameGeometry();
        if (frame.contains(cursor))
            return false;
        outsideNear = outsideNear || frame.adjusted(-OutsideMargin, -OutsideMargin, OutsideMargin, OutsideMargin).contains(cursor);
    }
    return outsideNear;
#endif
}

bool WindowVisibilityController::interactionInProgress() const
{
    if (belongsToGroup(QApplication::activePopupWidget()) || belongsToGroup(QApplication::activeModalWidget())
        || belongsToGroup(QWidget::mouseGrabber()))
        return true;
    QWidget *focus = QApplication::focusWidget();
    if (belongsToGroup(focus) && ((m_composingWidget && m_composingWidget == focus)
        || (QGuiApplication::inputMethod() && QGuiApplication::inputMethod()->isVisible())))
        return true;
    return m_native->busy();
}

void WindowVisibilityController::applyOpacity(QWidget *window)
{
    if (!window || m_applying)
        return;
    m_applying = true;
    if (!m_appliedWindows.contains(window))
        m_appliedWindows.append(window);
    const qreal opacity = m_autoHidden ? HiddenOpacity : m_opacityPercent / 100.0;
    window->setWindowOpacity(opacity);
    m_applying = false;
}

void WindowVisibilityController::applyOpacity()
{
    for (QWidget *window : groupWindows())
        applyOpacity(window);
}

void WindowVisibilityController::resetVisibility()
{
    m_autoHidden = false;
    m_lastNear = m_clock.elapsed();
    m_revealStarted = -1;
    applyOpacity();
}

void WindowVisibilityController::tick()
{
    const qint64 now = m_clock.elapsed();
    if (!m_autoHideEnabled || !m_host || !m_host->isVisible() || m_host->isMinimized()
        || interactionInProgress()) {
        resetVisibility();
        return;
    }
    if (m_autoHidden) {
        if (pointerIsNear(false)) {
            if (m_revealStarted < 0)
                m_revealStarted = now;
            else if (now - m_revealStarted >= VisibilityDelayMs)
                resetVisibility();
        } else {
            m_revealStarted = -1;
        }
    } else if (pointerIsNear(true)) {
        m_lastNear = now;
    } else if (now - m_lastNear >= VisibilityDelayMs) {
        m_autoHidden = true;
        m_revealStarted = -1;
        applyOpacity();
    }
}

bool WindowVisibilityController::eventFilter(QObject *watched, QEvent *event)
{
    QWidget *widget = qobject_cast<QWidget *>(watched);
    if (!widget || !belongsToGroup(widget))
        return QObject::eventFilter(watched, event);
    switch (event->type()) {
    case QEvent::Show:
    case QEvent::WinIdChange:
    case QEvent::ParentChange:
        // Applying before delivery of Show also prevents an independently
        // shown toolbar from flashing opaque while its host is invisible.
        if (widget->isWindow())
            applyOpacity(widget);
        break;
    case QEvent::Hide:
    case QEvent::WindowStateChange:
        if (widget == m_host && (!m_host->isVisible() || m_host->isMinimized()))
            resetVisibility();
        break;
    case QEvent::InputMethod: {
        const auto *input = static_cast<QInputMethodEvent *>(event);
        m_composingWidget = input->preeditString().isEmpty() ? nullptr : widget;
        break;
    }
    case QEvent::FocusOut:
        if (m_composingWidget == widget)
            m_composingWidget.clear();
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}
