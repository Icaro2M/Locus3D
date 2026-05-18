#pragma once

#include <GLFW/glfw3.h>

class WindowsWindowChrome
{
public:
    explicit WindowsWindowChrome(GLFWwindow* window);
    ~WindowsWindowChrome();

    void setTitleBarHeight(float height);
    void setTitleBarDragStartX(float x);
    void setTitleBarControlsWidth(float width);

    void minimize();
    void toggleMaximize();
    void close(GLFWwindow* window);

    bool isMaximized() const;

    static long long handleMessage(
        void* hwnd,
        unsigned int message,
        unsigned long long wParam,
        long long lParam,
        unsigned long long subclassId,
        unsigned long long refData
    );

private:
    void* m_hwnd;
    float m_titleBarHeight;
    float m_titleBarDragStartX;
    float m_titleBarControlsWidth;

    void configureNativeStyles();
    int hitTest(int screenX, int screenY) const;
    void refreshFrame();
};
