#include "CameraController.h"

#include <GLFW/glfw3.h>

CameraController::CameraController()
    : m_RotationSpeed(0.2f),
    m_ZoomSpeed(0.3f),
    m_MinPitch(-89.0f),
    m_MaxPitch(89.0f),
    m_MinDistance(0.5f),
    m_LastMouseX(0.0),
    m_LastMouseY(0.0),
    m_FirstMouse(true)
{
}

void CameraController::processMouse(GLFWwindow* window, Camera& camera)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (m_FirstMouse)
        {
            m_LastMouseX = mouseX;
            m_LastMouseY = mouseY;
            m_FirstMouse = false;
        }

        double deltaX = mouseX - m_LastMouseX;
        double deltaY = mouseY - m_LastMouseY;

        m_LastMouseX = mouseX;
        m_LastMouseY = mouseY;

        camera.setYaw(camera.getYaw() + static_cast<float>(deltaX) * m_RotationSpeed);
        camera.setPitch(camera.getPitch() - static_cast<float>(deltaY) * m_RotationSpeed);

        if (camera.getPitch() > m_MaxPitch)
            camera.setPitch(m_MaxPitch);

        if (camera.getPitch() < m_MinPitch)
            camera.setPitch(m_MinPitch);

        camera.updateOrbitPosition();
    }
    else
    {
        m_FirstMouse = true;
    }
}

void CameraController::processScroll(Camera& camera, float yOffset)
{
    camera.setDistance(camera.getDistance() - yOffset * m_ZoomSpeed);

    if (camera.getDistance() < m_MinDistance)
        camera.setDistance(m_MinDistance);

    camera.updateOrbitPosition();
}