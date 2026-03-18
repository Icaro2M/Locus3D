#pragma once

#include "Camera.h"
#include <GLFW/glfw3.h>

class CameraController
{
private:
    float m_RotationSpeed;
    float m_ZoomSpeed;
    float m_PanSpeed;

    float m_MinPitch;
    float m_MaxPitch;
    float m_MinDistance;

    double m_LastMouseX;
    double m_LastMouseY;
    bool m_FirstMouse;

public:
    CameraController();

    void processMouse(GLFWwindow* window, Camera& camera);
    void processScroll(Camera& camera, float yOffset);

};