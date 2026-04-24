#pragma once

#include "../../scene/Camera.h"
#include "../../scene/CameraController.h"
#include <GLFW/glfw3.h>


class CameraContext {
public:
    CameraContext();
    ~CameraContext() = default;

    void update(GLFWwindow* window);
    
    Camera& getCamera();
    CameraController& getController();

    static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

private:
    Camera m_camera;
    CameraController m_cameraController;
};