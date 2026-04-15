#pragma once

#include <glm/glm/glm.hpp>

#include "../../math/Ray.h"

class RotateDragInteraction
{
private:
    bool m_Active;
    glm::vec3 m_Origin;
    glm::vec3 m_AxisWorld;
    glm::vec3 m_PlaneNormal;
    glm::vec3 m_StartVector;
    float m_CurrentAngleDegrees;

private:
    bool intersectRayPlane(
        const Ray& ray,
        const glm::vec3& planePoint,
        const glm::vec3& planeNormal,
        glm::vec3& outPoint
    ) const;

public:
    RotateDragInteraction();

    bool begin(
        const glm::vec3& origin,
        const glm::vec3& axisWorld,
        const Ray& initialRay
    );

    void update(const Ray& currentRay);
    void reset();

    bool isActive() const;
    float getCurrentAngleDegrees() const;
};