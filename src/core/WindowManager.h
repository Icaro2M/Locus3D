#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>


class WindowManager
{
public:

    WindowManager(int width, int height, const char* title);
    ~WindowManager();

    bool shouldClose() const;

    void pollEvents();
    void swapBuffers();

    GLFWwindow* getWindow() const;

private:

    GLFWwindow* window;
    int width;
    int height;
};