#include "AxisDragInteraction.h"

#include <cmath>

AxisDragInteraction::AxisDragInteraction()
    : m_Active(false),
    m_Origin(0.0f, 0.0f, 0.0f),
    m_AxisWorld(0.0f, 0.0f, 0.0f),
    m_StartWorldPoint(0.0f, 0.0f, 0.0f),
    m_DragPlanePoint(0.0f, 0.0f, 0.0f),
    m_DragPlaneNormal(0.0f, 1.0f, 0.0f),
    m_CurrentDelta(0.0f)
{
}

bool AxisDragInteraction::intersectRayPlane(
    const Ray& ray,
    const glm::vec3& planePoint,
    const glm::vec3& planeNormal,
    glm::vec3& outPoint) const
{
    float denom = glm::dot(ray.direction, planeNormal);

    if (std::abs(denom) < 0.00001f)
    {
        return false;
    }

    float t = glm::dot(planePoint - ray.origin, planeNormal) / denom;

    if (t < 0.0f)
    {
        return false;
    }

    outPoint = ray.origin + ray.direction * t;
    return true;
}

bool AxisDragInteraction::begin(
    const glm::vec3& origin,
    const glm::vec3& axisWorld,
    const Ray& initialRay)
{
    if (glm::length(axisWorld) < 0.00001f)
    {
        return false;
    }

    glm::vec3 normalizedAxis = glm::normalize(axisWorld);
    glm::vec3 rayDir = glm::normalize(initialRay.direction);

    glm::vec3 helper = glm::cross(rayDir, normalizedAxis);

    if (glm::length(helper) < 0.00001f)
    {
        helper = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normalizedAxis);

        if (glm::length(helper) < 0.00001f)
        {
            helper = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), normalizedAxis);
        }
    }

    glm::vec3 planeNormal = glm::cross(normalizedAxis, helper);

    if (glm::length(planeNormal) < 0.00001f)
    {
        return false;
    }

    planeNormal = glm::normalize(planeNormal);

    glm::vec3 hitPoint(0.0f, 0.0f, 0.0f);
    if (!intersectRayPlane(initialRay, origin, planeNormal, hitPoint))
    {
        return false;
    }

    m_Active = true;
    m_Origin = origin;
    m_AxisWorld = normalizedAxis;
    m_StartWorldPoint = hitPoint;
    m_DragPlanePoint = origin;
    m_DragPlaneNormal = planeNormal;
    m_CurrentDelta = 0.0f;

    return true;
}

void AxisDragInteraction::update(const Ray& currentRay)
{
    if (!m_Active)
    {
        return;
    }

    glm::vec3 currentPoint(0.0f, 0.0f, 0.0f);
    if (!intersectRayPlane(currentRay, m_DragPlanePoint, m_DragPlaneNormal, currentPoint))
    {
        return;
    }

    glm::vec3 worldDelta = currentPoint - m_StartWorldPoint;
    m_CurrentDelta = glm::dot(worldDelta, m_AxisWorld);
}

void AxisDragInteraction::end()
{
    m_Active = false;
}

void AxisDragInteraction::reset()
{
    m_Active = false;
    m_Origin = glm::vec3(0.0f, 0.0f, 0.0f);
    m_AxisWorld = glm::vec3(0.0f, 0.0f, 0.0f);
    m_StartWorldPoint = glm::vec3(0.0f, 0.0f, 0.0f);
    m_DragPlanePoint = glm::vec3(0.0f, 0.0f, 0.0f);
    m_DragPlaneNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    m_CurrentDelta = 0.0f;
}

bool AxisDragInteraction::isActive() const
{
    return m_Active;
}

float AxisDragInteraction::getCurrentDelta() const
{
    return m_CurrentDelta;
}