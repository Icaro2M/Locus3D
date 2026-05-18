#pragma once

#include "../../core/WindowManager.h"

class WindowController
{
public:
    explicit WindowController(WindowManager* window);

    void setTitleBarMetrics(float height, float dragStartX, float controlsWidth);

    void minimize();
    void toggleMaximize();
    void close();

    bool isMaximized() const;

private:
    WindowManager* m_window;
};
