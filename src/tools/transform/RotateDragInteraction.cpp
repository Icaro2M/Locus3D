#include "RotateDragInteraction.h"

#include <cmath>

RotateDragInteraction::RotateDragInteraction()
    : m_Active(false),
    m_Origin(0.0f, 0.0f, 0.0f),
    m_AxisWorld(0.0f, 0.0f, 1.0f),
    m_PlaneNormal(0.0f, 0.0f, 1.0f),
    m_StartVector(1.0f, 0.0f, 0.0f),
    m_CurrentAngleDegrees(0.0f)
{
}

bool RotateDragInteraction::intersectRayPlane(
    const Ray& ray,
    const glm::vec3& planePoint,
    const glm::vec3& planeNormal,
    glm::vec3& outPoint
) const
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

bool RotateDragInteraction::begin(
    const glm::vec3& origin,
    const glm::vec3& axisWorld,
    const Ray& initialRay
)
{
    if (glm::dot(axisWorld, axisWorld) < 0.00001f)
    {
        return false;
    }

    glm::vec3 normalizedAxis = glm::normalize(axisWorld);
    glm::vec3 hitPoint(0.0f, 0.0f, 0.0f);

    if (!intersectRayPlane(initialRay, origin, normalizedAxis, hitPoint))
    {
        return false;
    }

    glm::vec3 startVector = hitPoint - origin;

    if (glm::dot(startVector, startVector) < 0.00001f)
    {
        return false;
    }

    m_Active = true;
    m_Origin = origin;
    m_AxisWorld = normalizedAxis;
    m_PlaneNormal = normalizedAxis;
    m_StartVector = glm::normalize(startVector);
    m_CurrentAngleDegrees = 0.0f;

    return true;
}

void RotateDragInteraction::update(const Ray& currentRay)
{
    if (!m_Active)
    {
        return;
    }

    glm::vec3 hitPoint(0.0f, 0.0f, 0.0f);

    if (!intersectRayPlane(currentRay, m_Origin, m_PlaneNormal, hitPoint))
    {
        return;
    }

    glm::vec3 currentVector = hitPoint - m_Origin;

    if (glm::dot(currentVector, currentVector) < 0.00001f)
    {
        return;
    }

    currentVector = glm::normalize(currentVector);

    float dotValue = glm::clamp(glm::dot(m_StartVector, currentVector), -1.0f, 1.0f);
    float angleRadians = std::acos(dotValue);

    glm::vec3 crossValue = glm::cross(m_StartVector, currentVector);
    float sign = glm::dot(crossValue, m_AxisWorld) >= 0.0f ? 1.0f : -1.0f;

    m_CurrentAngleDegrees = glm::degrees(angleRadians) * sign;
}

void RotateDragInteraction::reset()
{
    m_Active = false;
    m_Origin = glm::vec3(0.0f, 0.0f, 0.0f);
    m_AxisWorld = glm::vec3(0.0f, 0.0f, 1.0f);
    m_PlaneNormal = glm::vec3(0.0f, 0.0f, 1.0f);
    m_StartVector = glm::vec3(1.0f, 0.0f, 0.0f);
    m_CurrentAngleDegrees = 0.0f;
}

bool RotateDragInteraction::isActive() const
{
    return m_Active;
}

float RotateDragInteraction::getCurrentAngleDegrees() const
{
    return m_CurrentAngleDegrees;
}