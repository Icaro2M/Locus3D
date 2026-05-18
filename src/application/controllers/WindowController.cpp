#include "WindowController.h"

WindowController::WindowController(WindowManager* window)
    : m_window(window)
{
}

void WindowController::setTitleBarMetrics(float height, float dragStartX, float controlsWidth)
{
    if (m_window == nullptr)
    {
        return;
    }

    m_window->setCustomTitleBarHeight(height);
    m_window->setTitleBarDragStartX(dragStartX);
    m_window->setTitleBarControlsWidth(controlsWidth);
}

void WindowController::minimize()
{
    if (m_window != nullptr)
    {
        m_window->minimize();
    }
}

void WindowController::toggleMaximize()
{
    if (m_window != nullptr)
    {
        m_window->toggleMaximize();
    }
}

void WindowController::close()
{
    if (m_window != nullptr)
    {
        m_window->close();
    }
}

bool WindowController::isMaximized() const
{
    return m_window != nullptr && m_window->isMaximized();
}
