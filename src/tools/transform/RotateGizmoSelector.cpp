#include "RotateGizmoSelector.h"

#include <cmath>

RotateGizmoSelector::RotateGizmoSelector()
    : m_Radius(1.5f),
    m_Thickness(0.2f)
{
}

glm::vec3 RotateGizmoSelector::getAxisDirectionWorld(
    const SceneObject& object,
    TransformAxis axis,
    TransformSpace space
) const
{
    glm::vec3 axisDir(0.0f, 0.0f, 0.0f);

    switch (axis)
    {
    case TransformAxis::X:
        axisDir = glm::vec3(1.0f, 0.0f, 0.0f);
        break;
    case TransformAxis::Y:
        axisDir = glm::vec3(0.0f, 1.0f, 0.0f);
        break;
    case TransformAxis::Z:
        axisDir = glm::vec3(0.0f, 0.0f, 1.0f);
        break;
    default:
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    if (space == TransformSpace::Global)
    {
        return axisDir;
    }

    glm::mat4 model = object.getTransform().getModelMatrix();
    glm::mat3 rotation(model);

    glm::vec3 axisWorld = rotation * axisDir;

    if (glm::length(axisWorld) < 0.00001f)
    {
        return axisDir;
    }

    return glm::normalize(axisWorld);
}

TransformAxis RotateGizmoSelector::selectAxis(
    const SceneObject& object,
    GLFWwindow* window,
    const Camera& camera,
    TransformSpace space
) const
{
    Ray ray = m_Raycaster.buildRayFromMouse(window, camera);
    glm::vec3 origin = object.getTransform().getPosition();

    TransformAxis bestAxis = TransformAxis::None;
    float bestScore = 1000000.0f;

    TransformAxis axes[3] = {
        TransformAxis::X,
        TransformAxis::Y,
        TransformAxis::Z
    };

    for (TransformAxis axis : axes)
    {
        glm::vec3 normal = getAxisDirectionWorld(object, axis, space);

        float denom = glm::dot(ray.direction, normal);
        if (std::abs(denom) < 0.00001f)
        {
            continue;
        }

        float t = glm::dot(origin - ray.origin, normal) / denom;
        if (t < 0.0f)
        {
            continue;
        }

        glm::vec3 hitPoint = ray.origin + ray.direction * t;
        float distanceToCenter = glm::length(hitPoint - origin);
        float ringDistance = std::abs(distanceToCenter - m_Radius);

        if (ringDistance <= m_Thickness && ringDistance < bestScore)
        {
            bestScore = ringDistance;
            bestAxis = axis;
        }
    }

    return bestAxis;
}