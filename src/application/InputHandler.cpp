#include "InputHandler.h"
#include <imgui.h>

InputHandler::InputHandler()
{
    for (int i = 0; i < 512; ++i) m_keyStates[i] = false;
    for (int i = 0; i < 8; ++i) m_mouseStates[i] = false;
}

void InputHandler::process(GLFWwindow* window, AppEventBus* eventBus)
{
    ImGuiIO& io = ImGui::GetIO();

    if (!io.WantCaptureMouse)
    {
        bool leftMouse = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        if (leftMouse && !m_mouseStates[GLFW_MOUSE_BUTTON_LEFT])
        {
            eventBus->emit(EventType::InputMouseClickLeft);
        }
        else if (!leftMouse && m_mouseStates[GLFW_MOUSE_BUTTON_LEFT])
        {
            eventBus->emit(EventType::InputMouseReleaseLeft);
        }

        m_mouseStates[GLFW_MOUSE_BUTTON_LEFT] = leftMouse;
    }

    if (!io.WantCaptureKeyboard)
    {
        auto checkAndEmit = [&](int key, EventType eventType) {
            bool pressed = glfwGetKey(window, key) == GLFW_PRESS;

            if (pressed && !m_keyStates[key])
            {
                eventBus->emit(eventType);
            }

            m_keyStates[key] = pressed;
            };

        auto updateKeyStateOnly = [&](int key) {
            bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
            m_keyStates[key] = pressed;
            };

        bool ctrlPressed =
            glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

        checkAndEmit(GLFW_KEY_W, EventType::InputKeyW);
        checkAndEmit(GLFW_KEY_E, EventType::InputKeyE);
        checkAndEmit(GLFW_KEY_R, EventType::InputKeyR);
        checkAndEmit(GLFW_KEY_G, EventType::InputKeyG);
        checkAndEmit(GLFW_KEY_L, EventType::InputKeyL);
        checkAndEmit(GLFW_KEY_F, EventType::InputKeyF);
        checkAndEmit(GLFW_KEY_T, EventType::InputKeyT);
        checkAndEmit(GLFW_KEY_M, EventType::InputKeyM);

        if (ctrlPressed)
        {
            updateKeyStateOnly(GLFW_KEY_S);
        }
        else
        {
            checkAndEmit(GLFW_KEY_S, EventType::InputKeyS);
        }

        checkAndEmit(GLFW_KEY_ESCAPE, EventType::InputKeyEscape);
        checkAndEmit(GLFW_KEY_ENTER, EventType::InputKeyEnter);
    }
}