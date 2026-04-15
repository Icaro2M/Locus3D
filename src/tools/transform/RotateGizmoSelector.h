#pragma once

struct GLFWwindow;

#include <glm/glm/glm.hpp>

#include "../../scene/SceneObject.h"
#include "../../scene/Camera.h"
#include "../selection/Raycaster.h"
#include "TransformTypes.h"

class RotateGizmoSelector
{
private:
    Raycaster m_Raycaster;
    float m_Radius;
    float m_Thickness;

private:
    glm::vec3 getAxisDirectionWorld(
        const SceneObject& object,
        TransformAxis axis,
        TransformSpace space
    ) const;

public:
    RotateGizmoSelector();

    TransformAxis selectAxis(
        const SceneObject& object,
        GLFWwindow* window,
        const Camera& camera,
        TransformSpace space
    ) const;
};