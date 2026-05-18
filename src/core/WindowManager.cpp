#include "WindowManager.h"

#include "../platform/windows/WindowsWindowChrome.h"

#include <algorithm>
#include <utility>

WindowManager::WindowManager(int width, int height, const char* title)
    : window(nullptr),
    width(width),
    height(height),
    customTitleBarHeight(0.0f),
    titleBarDragStartX(0.0f),
    titleBarControlsWidth(0.0f),
    previousScrollCallback(nullptr),
    previousRefreshCallback(nullptr)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }

    glEnable(GL_MULTISAMPLE);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    installNativeWindowChrome();

    int framebufferWidth = 0;
    int framebufferHeight = 0;

    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    this->width = framebufferWidth;
    this->height = framebufferHeight;

    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

WindowManager::~WindowManager()
{
    windowChrome.reset();

    glfwDestroyWindow(window);
    glfwTerminate();
}

bool WindowManager::shouldClose() const
{
    return glfwWindowShouldClose(window);
}

void WindowManager::pollEvents()
{
    glfwPollEvents();
}

void WindowManager::swapBuffers()
{
    glfwSwapBuffers(window);
}

GLFWwindow* WindowManager::getWindow() const
{
    return window;
}

void WindowManager::setScrollCallback(std::function<void(double, double)> callback)
{
    scrollCallbackHandler = std::move(callback);
    previousScrollCallback = glfwSetScrollCallback(window, scrollCallback);
}

void WindowManager::setRefreshCallback(std::function<void()> callback)
{
    refreshCallbackHandler = std::move(callback);
    previousRefreshCallback = glfwSetWindowRefreshCallback(window, refreshCallback);
}

void WindowManager::setCustomTitleBarHeight(float height)
{
    customTitleBarHeight = std::max(0.0f, height);

    if (windowChrome)
    {
        windowChrome->setTitleBarHeight(customTitleBarHeight);
    }
}

void WindowManager::setTitleBarDragStartX(float x)
{
    titleBarDragStartX = std::max(0.0f, x);

    if (windowChrome)
    {
        windowChrome->setTitleBarDragStartX(titleBarDragStartX);
    }
}

void WindowManager::setTitleBarControlsWidth(float width)
{
    titleBarControlsWidth = std::max(0.0f, width);

    if (windowChrome)
    {
        windowChrome->setTitleBarControlsWidth(titleBarControlsWidth);
    }
}

void WindowManager::minimize()
{
    if (windowChrome)
    {
        windowChrome->minimize();
    }
    else
    {
        glfwIconifyWindow(window);
    }
}

void WindowManager::toggleMaximize()
{
    if (windowChrome)
    {
        windowChrome->toggleMaximize();
    }
    else
    {
        if (isMaximized())
        {
            glfwRestoreWindow(window);
        }
        else
        {
            glfwMaximizeWindow(window);
        }
    }
}

void WindowManager::close()
{
    if (windowChrome)
    {
        windowChrome->close(window);
    }
    else
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

bool WindowManager::isMaximized() const
{
    if (windowChrome)
    {
        return windowChrome->isMaximized();
    }

    return glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
}

int WindowManager::getWidth() const
{
    return width;
}

int WindowManager::getHeight() const
{
    return height;
}

float WindowManager::getAspectRatio() const
{
    if (height == 0)
    {
        return 1.0f;
    }

    return static_cast<float>(width) / static_cast<float>(height);
}

void WindowManager::framebufferSizeCallback(GLFWwindow* glfwWindow, int newWidth, int newHeight)
{
    WindowManager* manager =
        static_cast<WindowManager*>(glfwGetWindowUserPointer(glfwWindow));

    if (manager != nullptr)
    {
        manager->width = newWidth;
        manager->height = newHeight;
    }

    glViewport(0, 0, newWidth, newHeight);
}

void WindowManager::scrollCallback(GLFWwindow* glfwWindow, double xOffset, double yOffset)
{
    WindowManager* manager =
        static_cast<WindowManager*>(glfwGetWindowUserPointer(glfwWindow));

    if (manager != nullptr && manager->scrollCallbackHandler)
    {
        if (manager->previousScrollCallback != nullptr)
        {
            manager->previousScrollCallback(glfwWindow, xOffset, yOffset);
        }

        manager->scrollCallbackHandler(xOffset, yOffset);
    }
}

void WindowManager::refreshCallback(GLFWwindow* glfwWindow)
{
    WindowManager* manager =
        static_cast<WindowManager*>(glfwGetWindowUserPointer(glfwWindow));

    if (manager != nullptr && manager->refreshCallbackHandler)
    {
        if (manager->previousRefreshCallback != nullptr)
        {
            manager->previousRefreshCallback(glfwWindow);
        }

        manager->refreshCallbackHandler();
    }
}

void WindowManager::installNativeWindowChrome()
{
#ifdef _WIN32
    windowChrome = std::make_unique<WindowsWindowChrome>(window);
    windowChrome->setTitleBarHeight(customTitleBarHeight);
    windowChrome->setTitleBarDragStartX(titleBarDragStartX);
    windowChrome->setTitleBarControlsWidth(titleBarControlsWidth);
#endif
}
