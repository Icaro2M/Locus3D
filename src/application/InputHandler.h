#pragma once

#include "AppEventBus.h"
#include <GLFW/glfw3.h>

class InputHandler {
public:
    InputHandler();
    ~InputHandler() = default;

    void process(GLFWwindow* window, AppEventBus* eventBus);

private:
    bool m_keyStates[512];
    bool m_mouseStates[8];
};