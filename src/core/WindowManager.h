#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

class WindowManager
{
public:
    WindowManager(int width, int height, const char* title);
    ~WindowManager();

    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();

    GLFWwindow* getWindow() const;

    int getWidth() const;
    int getHeight() const;
    float getAspectRatio() const;

private:
    GLFWwindow* window;

    int width;
    int height;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};