#pragma once

struct GLFWwindow;

#include "../../scene/SceneObject.h"
#include "../../scene/Camera.h"
#include "../selection/Raycaster.h"
#include "TransformController.h"

class ScaleGizmoSelector
{
private:
    Raycaster m_Raycaster;
    float m_AxisLength;
    float m_HandleHalfSize;

private:
    bool rayIntersectsAABB(
        const Ray& ray,
        const glm::vec3& min,
        const glm::vec3& max,
        float& tOut) const;

    void buildHandleAABB(
        const glm::vec3& center,
        glm::vec3& outMin,
        glm::vec3& outMax) const;

public:
    ScaleGizmoSelector();

    TransformAxis selectAxis(
        const SceneObject& object,
        GLFWwindow* window,
        const Camera& camera,
        TransformSpace space) const;
};