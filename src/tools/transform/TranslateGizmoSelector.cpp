#include "TranslateGizmoSelector.h"

#include <algorithm>
#include <limits>
#include <cmath>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

TranslateGizmoSelector::TranslateGizmoSelector()
    : m_AxisLength(1.5f),
    m_AxisThickness(0.1f)
{
}

bool TranslateGizmoSelector::rayIntersectsAABB(
    const Ray& ray,
    const glm::vec3& min,
    const glm::vec3& max,
    float& tOut) const
{
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();

    for (int i = 0; i < 3; i++)
    {
        if (std::abs(ray.direction[i]) < 0.00001f)
        {
            if (ray.origin[i] < min[i] || ray.origin[i] > max[i])
            {
                return false;
            }
        }
        else
        {
            float ood = 1.0f / ray.direction[i];
            float t1 = (min[i] - ray.origin[i]) * ood;
            float t2 = (max[i] - ray.origin[i]) * ood;

            if (t1 > t2)
            {
                std::swap(t1, t2);
            }

            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            if (tMin > tMax)
            {
                return false;
            }
        }
    }

    tOut = tMin;
    return true;
}

void TranslateGizmoSelector::buildAxisAABB(
    const glm::vec3& origin,
    const glm::vec3& axis,
    glm::vec3& outMin,
    glm::vec3& outMax) const
{
    glm::vec3 end = origin + axis * m_AxisLength;

    glm::vec3 min = glm::min(origin, end);
    glm::vec3 max = glm::max(origin, end);

    min -= glm::vec3(m_AxisThickness);
    max += glm::vec3(m_AxisThickness);

    outMin = min;
    outMax = max;
}

TransformAxis TranslateGizmoSelector::selectAxis(
    const SceneObject& object,
    GLFWwindow* window,
    const Camera& camera,
    TransformSpace space) const
{
    Ray ray = m_Raycaster.buildRayFromMouse(window, camera);

    const glm::vec3 origin = object.getTransform().getPosition();

    glm::vec3 axisX(1.0f, 0.0f, 0.0f);
    glm::vec3 axisY(0.0f, 1.0f, 0.0f);
    glm::vec3 axisZ(0.0f, 0.0f, 1.0f);

    if (space == TransformSpace::Local)
    {
        glm::mat4 model = object.getTransform().getModelMatrix();
        glm::mat3 rotation(model);

        axisX = glm::normalize(rotation * axisX);
        axisY = glm::normalize(rotation * axisY);
        axisZ = glm::normalize(rotation * axisZ);
    }

    glm::vec3 minX, maxX;
    glm::vec3 minY, maxY;
    glm::vec3 minZ, maxZ;

    buildAxisAABB(origin, axisX, minX, maxX);
    buildAxisAABB(origin, axisY, minY, maxY);
    buildAxisAABB(origin, axisZ, minZ, maxZ);

    float tX, tY, tZ;
    bool hitX = rayIntersectsAABB(ray, minX, maxX, tX);
    bool hitY = rayIntersectsAABB(ray, minY, maxY, tY);
    bool hitZ = rayIntersectsAABB(ray, minZ, maxZ, tZ);

    float bestT = std::numeric_limits<float>::max();
    TransformAxis bestAxis = TransformAxis::None;

    if (hitX && tX < bestT)
    {
        bestT = tX;
        bestAxis = TransformAxis::X;
    }

    if (hitY && tY < bestT)
    {
        bestT = tY;
        bestAxis = TransformAxis::Y;
    }

    if (hitZ && tZ < bestT)
    {
        bestT = tZ;
        bestAxis = TransformAxis::Z;
    }

    return bestAxis;
}