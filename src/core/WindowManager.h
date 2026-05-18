#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <iostream>
#include <memory>

class WindowsWindowChrome;

class WindowManager
{
public:
    WindowManager(int width, int height, const char* title);
    ~WindowManager();

    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();

    GLFWwindow* getWindow() const;

    void setScrollCallback(std::function<void(double, double)> callback);
    void setRefreshCallback(std::function<void()> callback);
    void setCustomTitleBarHeight(float height);
    void setTitleBarDragStartX(float x);
    void setTitleBarControlsWidth(float width);

    void minimize();
    void toggleMaximize();
    void close();
    bool isMaximized() const;

    int getWidth() const;
    int getHeight() const;
    float getAspectRatio() const;

private:
    GLFWwindow* window;

    int width;
    int height;

    float customTitleBarHeight;
    float titleBarDragStartX;
    float titleBarControlsWidth;

    std::function<void(double, double)> scrollCallbackHandler;
    GLFWscrollfun previousScrollCallback;
    std::function<void()> refreshCallbackHandler;
    GLFWwindowrefreshfun previousRefreshCallback;

    std::unique_ptr<WindowsWindowChrome> windowChrome;

    void installNativeWindowChrome();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static void refreshCallback(GLFWwindow* window);
};
