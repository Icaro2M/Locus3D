#include "CameraContext.h"

CameraContext::CameraContext()
{
m_camera.setPosition(glm::vec3(0.0f, 5.0f, 18.0f));}

void CameraContext::update(GLFWwindow* window)
{
    m_cameraController.processMouse(window, m_camera);
}

Camera& CameraContext::getCamera()
{
    return m_camera;
}

CameraController& CameraContext::getController()
{
    return m_cameraController;
}

void CameraContext::scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    CameraContext* instance = static_cast<CameraContext*>(glfwGetWindowUserPointer(window));
    if (instance)
    {
        instance->getController().processScroll(instance->getCamera(), static_cast<float>(yOffset));
    }
}

void CameraContext::resize(int width, int height)
{
    if (height <= 0)
    {
        return;
    }

    m_camera.setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
}