#include "WindowManager.h"
#include <iostream>

WindowManager::WindowManager(int width, int height, const char* title)
    : width(width), height(height), window(nullptr)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    glViewport(0, 0, width, height);
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