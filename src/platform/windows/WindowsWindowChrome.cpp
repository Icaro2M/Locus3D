#include "WindowsWindowChrome.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <GLFW/glfw3native.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <windowsx.h>
#endif

#include <algorithm>

#ifdef _WIN32
namespace
{
    constexpr UINT_PTR WindowChromeSubclassId = 1;
    constexpr int ResizeBorderWidth = 8;

    LRESULT CALLBACK WindowChromeProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam,
        UINT_PTR subclassId,
        DWORD_PTR refData
    )
    {
        return static_cast<LRESULT>(
            WindowsWindowChrome::handleMessage(
                hwnd,
                message,
                static_cast<unsigned long long>(wParam),
                static_cast<long long>(lParam),
                static_cast<unsigned long long>(subclassId),
                static_cast<unsigned long long>(refData)
            )
        );
    }
}
#endif

WindowsWindowChrome::WindowsWindowChrome(GLFWwindow* window)
    : m_hwnd(nullptr),
    m_titleBarHeight(0.0f),
    m_titleBarDragStartX(0.0f),
    m_titleBarControlsWidth(0.0f)
{
#ifdef _WIN32
    if (window == nullptr)
    {
        return;
    }

    m_hwnd = glfwGetWin32Window(window);

    if (m_hwnd == nullptr)
    {
        return;
    }

    configureNativeStyles();

    SetWindowSubclass(
        static_cast<HWND>(m_hwnd),
        WindowChromeProc,
        WindowChromeSubclassId,
        reinterpret_cast<DWORD_PTR>(this)
    );
#endif
}

WindowsWindowChrome::~WindowsWindowChrome()
{
#ifdef _WIN32
    if (m_hwnd != nullptr)
    {
        RemoveWindowSubclass(
            static_cast<HWND>(m_hwnd),
            WindowChromeProc,
            WindowChromeSubclassId
        );
    }
#endif
}

void WindowsWindowChrome::setTitleBarHeight(float height)
{
    m_titleBarHeight = std::max(0.0f, height);
}

void WindowsWindowChrome::setTitleBarDragStartX(float x)
{
    m_titleBarDragStartX = std::max(0.0f, x);
}

void WindowsWindowChrome::setTitleBarControlsWidth(float width)
{
    m_titleBarControlsWidth = std::max(0.0f, width);
}

void WindowsWindowChrome::minimize()
{
#ifdef _WIN32
    if (m_hwnd != nullptr)
    {
        PostMessage(static_cast<HWND>(m_hwnd), WM_SYSCOMMAND, SC_MINIMIZE, 0);
    }
#endif
}

void WindowsWindowChrome::toggleMaximize()
{
#ifdef _WIN32
    if (m_hwnd == nullptr)
    {
        return;
    }

    PostMessage(
        static_cast<HWND>(m_hwnd),
        WM_SYSCOMMAND,
        isMaximized() ? SC_RESTORE : SC_MAXIMIZE,
        0
    );
#endif
}

void WindowsWindowChrome::close(GLFWwindow* window)
{
    if (window != nullptr)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

bool WindowsWindowChrome::isMaximized() const
{
#ifdef _WIN32
    return m_hwnd != nullptr && IsZoomed(static_cast<HWND>(m_hwnd)) != FALSE;
#else
    return false;
#endif
}

void WindowsWindowChrome::configureNativeStyles()
{
#ifdef _WIN32
    if (m_hwnd == nullptr)
    {
        return;
    }

    HWND hwnd = static_cast<HWND>(m_hwnd);
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);

    style |= WS_CAPTION |
        WS_THICKFRAME |
        WS_SYSMENU |
        WS_MINIMIZEBOX |
        WS_MAXIMIZEBOX;

    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    refreshFrame();
#endif
}

void WindowsWindowChrome::refreshFrame()
{
#ifdef _WIN32
    if (m_hwnd == nullptr)
    {
        return;
    }

    SetWindowPos(
        static_cast<HWND>(m_hwnd),
        nullptr,
        0,
        0,
        0,
        0,
        SWP_FRAMECHANGED |
        SWP_NOMOVE |
        SWP_NOSIZE |
        SWP_NOZORDER |
        SWP_NOOWNERZORDER
    );
#endif
}

int WindowsWindowChrome::hitTest(int screenX, int screenY) const
{
#ifdef _WIN32
    if (m_hwnd == nullptr)
    {
        return HTNOWHERE;
    }

    HWND hwnd = static_cast<HWND>(m_hwnd);
    POINT cursor = { screenX, screenY };
    ScreenToClient(hwnd, &cursor);

    RECT clientRect {};
    GetClientRect(hwnd, &clientRect);

    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    const int resizeBorder = isMaximized() ? 0 : ResizeBorderWidth;

    const bool insideClient =
        cursor.x >= 0 &&
        cursor.x <= clientWidth &&
        cursor.y >= 0 &&
        cursor.y <= clientHeight;

    if (!insideClient)
    {
        return HTNOWHERE;
    }

    const bool onLeft = cursor.x < resizeBorder;
    const bool onRight = cursor.x >= clientWidth - resizeBorder;
    const bool onTop = cursor.y < resizeBorder;
    const bool onBottom = cursor.y >= clientHeight - resizeBorder;

    if (onTop && onLeft)
    {
        return HTTOPLEFT;
    }

    if (onTop && onRight)
    {
        return HTTOPRIGHT;
    }

    if (onBottom && onLeft)
    {
        return HTBOTTOMLEFT;
    }

    if (onBottom && onRight)
    {
        return HTBOTTOMRIGHT;
    }

    if (onLeft)
    {
        return HTLEFT;
    }

    if (onRight)
    {
        return HTRIGHT;
    }

    if (onTop)
    {
        return HTTOP;
    }

    if (onBottom)
    {
        return HTBOTTOM;
    }

    const int titleBarHeight = static_cast<int>(m_titleBarHeight);
    const int dragStartX = static_cast<int>(m_titleBarDragStartX);
    const int controlsStartX = clientWidth - static_cast<int>(m_titleBarControlsWidth);

    if (cursor.y >= 0 &&
        cursor.y < titleBarHeight &&
        cursor.x >= dragStartX &&
        cursor.x < controlsStartX)
    {
        return HTCAPTION;
    }
#endif

    return HTNOWHERE;
}

long long WindowsWindowChrome::handleMessage(
    void* hwnd,
    unsigned int message,
    unsigned long long wParam,
    long long lParam,
    unsigned long long,
    unsigned long long refData
)
{
#ifdef _WIN32
    WindowsWindowChrome* chrome =
        reinterpret_cast<WindowsWindowChrome*>(static_cast<DWORD_PTR>(refData));

    switch (message)
    {
    case WM_NCCALCSIZE:
        if (wParam == TRUE)
        {
            NCCALCSIZE_PARAMS* params =
                reinterpret_cast<NCCALCSIZE_PARAMS*>(static_cast<LPARAM>(lParam));

            if (params != nullptr && IsZoomed(static_cast<HWND>(hwnd)) != FALSE)
            {
                MONITORINFO monitorInfo {};
                monitorInfo.cbSize = sizeof(MONITORINFO);

                HMONITOR monitor = MonitorFromWindow(
                    static_cast<HWND>(hwnd),
                    MONITOR_DEFAULTTONEAREST
                );

                if (GetMonitorInfo(monitor, &monitorInfo) != FALSE)
                {
                    params->rgrc[0] = monitorInfo.rcWork;
                }
            }

            return 0;
        }

        break;

    case WM_NCACTIVATE:
        return TRUE;

    case WM_NCHITTEST:
        if (chrome != nullptr)
        {
            const int hit = chrome->hitTest(
                GET_X_LPARAM(static_cast<LPARAM>(lParam)),
                GET_Y_LPARAM(static_cast<LPARAM>(lParam))
            );

            if (hit != HTNOWHERE)
            {
                return hit;
            }
        }

        break;

    case WM_SIZING:
    case WM_SIZE:
        InvalidateRect(static_cast<HWND>(hwnd), nullptr, FALSE);
        UpdateWindow(static_cast<HWND>(hwnd));
        break;

    case WM_GETMINMAXINFO:
        if (chrome != nullptr && chrome->isMaximized())
        {
            return 0;
        }

        break;

    default:
        break;
    }

    return DefSubclassProc(
        static_cast<HWND>(hwnd),
        static_cast<UINT>(message),
        static_cast<WPARAM>(wParam),
        static_cast<LPARAM>(lParam)
    );
#else
    (void)hwnd;
    (void)message;
    (void)wParam;
    (void)lParam;
    (void)refData;
    return 0;
#endif
}
