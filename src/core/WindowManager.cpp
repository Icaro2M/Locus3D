#include "WindowManager.h"

WindowManager::WindowManager(int width, int height, const char* title)
    : window(nullptr), width(width), height(height)
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

    int framebufferWidth = 0;
    int framebufferHeight = 0;

    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    this->width = framebufferWidth;
    this->height = framebufferHeight;

    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

WindowManager::~WindowManager()
{
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