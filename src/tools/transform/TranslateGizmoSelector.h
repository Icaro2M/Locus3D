#pragma once

struct GLFWwindow;

#include "../../scene/SceneObject.h"
#include "../../scene/Camera.h"
#include "../selection/Raycaster.h"
#include "TransformController.h"

class TranslateGizmoSelector
{
private:
    Raycaster m_Raycaster;
    float m_AxisLength;
    float m_AxisThickness;

private:
    bool rayIntersectsAABB(
        const Ray& ray,
        const glm::vec3& min,
        const glm::vec3& max,
        float& tOut) const;

    void buildAxisAABB(
        const glm::vec3& origin,
        const glm::vec3& axis,
        glm::vec3& outMin,
        glm::vec3& outMax) const;

public:
    TranslateGizmoSelector();

    TransformAxis selectAxis(
        const SceneObject& object,
        GLFWwindow* window,
        const Camera& camera,
        TransformSpace space) const;
};