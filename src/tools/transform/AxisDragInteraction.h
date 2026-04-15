#pragma once

#include <glm/glm/glm.hpp>

#include "../../math/Ray.h"

class AxisDragInteraction
{
private:
    bool m_Active;
    glm::vec3 m_Origin;
    glm::vec3 m_AxisWorld;
    glm::vec3 m_StartWorldPoint;
    glm::vec3 m_DragPlanePoint;
    glm::vec3 m_DragPlaneNormal;
    float m_CurrentDelta;

private:
    bool intersectRayPlane(
        const Ray& ray,
        const glm::vec3& planePoint,
        const glm::vec3& planeNormal,
        glm::vec3& outPoint) const;

public:
    AxisDragInteraction();

    bool begin(
        const glm::vec3& origin,
        const glm::vec3& axisWorld,
        const Ray& initialRay);

    void update(const Ray& currentRay);

    void end();
    void reset();

    bool isActive() const;
    float getCurrentDelta() const;
};