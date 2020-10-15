#include "windownativeeventfilter.h"

#if defined(Q_OS_WIN)
#include <QCursor>
#include <QDebug>
#include <QGuiApplication>
#include <windows.h>
#include <windowsx.h>

WindowNativeEventFilter::WindowNativeEventFilter() {}

WindowNativeEventFilter::~WindowNativeEventFilter()
{
    // do nothing, because this object is static instance
}

WindowNativeEventFilter *WindowNativeEventFilter::Instance()
{
    static WindowNativeEventFilter g_windowNativeEventFilter;
    return &g_windowNativeEventFilter;
}

void WindowNativeEventFilter::Init()
{
    if (!m_inited) {
        m_inited = true;
        QGuiApplication::instance()->installNativeEventFilter(this);
    }
}

bool WindowNativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);

    MSG *param = static_cast<MSG *>(message);
    if (!param) {
        return false;
    }

    switch (param->message) {
    case WM_NCHITTEST: {
        if (processNcHitTest(message, result)) {
            return true;
        }
    } break;
    case WM_NCLBUTTONDOWN: {
        if (processNcLButtonDown(message, result)) {
            return true;
        }
    } break;
    case WM_SETCURSOR: {
        if (processSetCursor(message, result)) {
            return true;
        }
    } break;
    default:
        break;
    }

    return false;
}

bool WindowNativeEventFilter::processNcHitTest(void *message, long *result)
{
    MSG *param = static_cast<MSG *>(message);
    if (!param) {
        return false;
    }

    QWindow *window = getWindow((WId)param->hwnd);
    if (!window) {
        return false;
    }

    // 没有最大化&全屏按钮，则认为是固定尺寸窗口
    if (!(window->flags() & Qt::WindowMaximizeButtonHint) && !(window->flags() & Qt::WindowFullscreenButtonHint)) {
        return false;
    }

    qreal dpi = window->devicePixelRatio();
    if (dpi <= 0.99) {
        dpi = 1.0;
    }

    QPoint ptCursor = QCursor::pos();
    int nX = ptCursor.x() - window->geometry().x();
    int nY = ptCursor.y() - window->geometry().y();

    *result = HTCLIENT;

    if (Qt::WindowMaximized == window->windowState()) {
        return false;
    }

    if ((window->windowStates() & Qt::WindowMaximized) || (window->windowStates() & Qt::WindowFullScreen)) {
        return false;
    }

    qDebug() << "processNcHitTest:" << ptCursor << window->geometry() << "d:" << nX;
    int borderWidth = 5;
    if ((nX > m_windowMargin.left()) && (nX < m_windowMargin.left() + borderWidth) && (nY > m_windowMargin.top())
        && (nY < m_windowMargin.top() + borderWidth)) {
        *result = HTTOPLEFT;
    } else if (
        (nX > window->width() - m_windowMargin.right() - borderWidth) && (nX < window->width() - m_windowMargin.right()) && (nY > m_windowMargin.top())
        && (nY < m_windowMargin.top() + borderWidth)) {
        *result = HTTOPRIGHT;
    } else if (
        (nX > m_windowMargin.left()) && (nX < m_windowMargin.left() + borderWidth) && (nY > window->height() - m_windowMargin.bottom() - borderWidth)
        && (nY < window->height() - m_windowMargin.bottom())) {
        *result = HTBOTTOMLEFT;
    } else if (
        (nX > window->width() - m_windowMargin.right() - borderWidth) && (nX < window->width() - m_windowMargin.right())
        && (nY > window->height() - m_windowMargin.bottom() - borderWidth) && (nY < window->height() - m_windowMargin.bottom())) {
        *result = HTBOTTOMRIGHT;
    } else if ((nX > m_windowMargin.left()) && (nX < m_windowMargin.left() + borderWidth)) {
        *result = HTLEFT;
    } else if ((nX > window->width() - m_windowMargin.right() - borderWidth) && (nX < window->width() - m_windowMargin.right())) {
        *result = HTRIGHT;
    } else if ((nY > m_windowMargin.top()) && (nY < m_windowMargin.top() + borderWidth)) {
        *result = HTTOP;
    } else if ((nY > window->height() - m_windowMargin.bottom() - borderWidth) && (nY < window->height() - m_windowMargin.bottom())) {
        *result = HTBOTTOM;
    }

    return true;
}

bool WindowNativeEventFilter::processNcLButtonDown(void *message, long *result)
{
    Q_UNUSED(result);

    MSG *param = static_cast<MSG *>(message);
    if (!param) {
        return false;
    }

    QWindow *window = getWindow((WId)param->hwnd);
    if (!window) {
        return false;
    }

    if (!(window->flags() & Qt::WindowMaximizeButtonHint) && !(window->flags() & Qt::WindowFullscreenButtonHint)) {
        return false;
    }

    if ((window->windowStates() & Qt::WindowMaximized) || (window->windowStates() & Qt::WindowFullScreen)) {
        return false;
    }

    HWND hwnd = param->hwnd;
    WPARAM nHitTest = param->wParam;
    if (nHitTest == HTTOP) {
        ::PostMessage(hwnd, WM_SYSCOMMAND, SC_SIZE | WMSZ_TOP, param->lParam);
    } else if (nHitTest == HTBOTTOM) {
        ::PostMessage(hwnd, WM_SYSCOMMAND, SC_SIZE | WMSZ_BOTTOM, param->lParam);
    } else if (nHitTest == HTLEFT) {
        ::PostMessage(hwnd, WM_SYSCOMMAND, SC_SIZE | WMSZ_LEFT, param->lParam);
    } else if (nHitTest == HTRIGHT) {
        ::PostMessage(hwnd, WM_SYSCOMMAND, SC_SIZE | WMSZ_RIGHT, param->lParam);
    } else if (nHitTest == HTTOPLEFT) {
        ::PostMessage(hwnd, WM_SYSCOMMAND, SC_SIZE | WMSZ_TOPLEFT, param->lParam);
    } else if (nHitTest == HTTOPRIGHT) {
        ::PostMessage(hwnd, WM_SYSCOMMAND, SC_SIZE | WMSZ_TOPRIGHT, param->lParam);
    } else if (nHitTest == HTBOTTOMLEFT) {
        ::PostMessage(hwnd, WM_SYSCOMMAND, SC_SIZE | WMSZ_BOTTOMLEFT, param->lParam);
    } else if (nHitTest == HTBOTTOMRIGHT) {
        ::PostMessage(hwnd, WM_SYSCOMMAND, SC_SIZE | WMSZ_BOTTOMRIGHT, param->lParam);
    } else if (nHitTest == HTCAPTION) {
        ::PostMessage(hwnd, WM_SYSCOMMAND, SC_MOVE + 1, 0);
    }

    return false;
}

bool WindowNativeEventFilter::processSetCursor(void *message, long *result)
{
    Q_UNUSED(result);

    MSG *param = static_cast<MSG *>(message);
    if (!param) {
        return false;
    }

    QWindow *window = getWindow((WId)param->hwnd);
    if (!window) {
        return false;
    }

    if (!(window->flags() & Qt::WindowMaximizeButtonHint) && !(window->flags() & Qt::WindowFullscreenButtonHint)) {
        return false;
    }

    if ((window->windowStates() & Qt::WindowMaximized) || (window->windowStates() & Qt::WindowFullScreen)) {
        return false;
    }

    // is invisible window
    HWND hwnd = param->hwnd;
    if (!::IsWindowVisible(hwnd)) {
        return true;
    }

    // is not enabled window
    if (!::IsWindowEnabled(hwnd)) {
        return true;
    }

    // set cursor
    HCURSOR hCursor = nullptr;
    LPARAM hittest = LOWORD(param->lParam);
    switch (hittest) {
    case HTRIGHT:
    case HTLEFT:
        hCursor = LoadCursor(nullptr, IDC_SIZEWE);
        break;
    case HTTOP:
    case HTBOTTOM:
        hCursor = LoadCursor(nullptr, IDC_SIZENS);
        break;
    case HTTOPLEFT:
    case HTBOTTOMRIGHT:
        hCursor = LoadCursor(nullptr, IDC_SIZENWSE);
        break;
    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
        hCursor = LoadCursor(nullptr, IDC_SIZENESW);
        break;
    default:
        break;
    }

    if (!hCursor) {
        return false;
    }

    ::SetCursor(hCursor);
    ::DestroyCursor(hCursor);

    return true;
}

QWindow *WindowNativeEventFilter::getWindow(WId wndId)
{
    QWindowList windows = QGuiApplication::topLevelWindows();
    for (int i = 0; i < windows.size(); ++i) {
        if (windows.at(i)->winId() == wndId) {
            return windows.at(i);
        }
    }
    return nullptr;
}

#endif //(Q_OS_WIN)
