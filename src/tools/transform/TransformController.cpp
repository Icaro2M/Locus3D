#include "TransformController.h"

#include <iostream>
#include <cmath>

TransformController::TransformController()
    : m_SelectedObject(nullptr),
    m_Mode(TransformMode::None),
    m_Axis(TransformAxis::None),
    m_Space(TransformSpace::Global),
    m_IsDragging(false),
    m_DragStartPosition(0.0f, 0.0f, 0.0f),
    m_DragStartWorldPoint(0.0f, 0.0f, 0.0f),
    m_DragAxisWorld(0.0f, 0.0f, 0.0f),
    m_DragPlanePoint(0.0f, 0.0f, 0.0f),
    m_DragPlaneNormal(0.0f, 1.0f, 0.0f)
{
}

void TransformController::setSelectedObject(SceneObject* object)
{
    m_SelectedObject = object;
}

SceneObject* TransformController::getSelectedObject() const
{
    return m_SelectedObject;
}

void TransformController::setMode(TransformMode mode)
{
    m_Mode = mode;
}

TransformMode TransformController::getMode() const
{
    return m_Mode;
}

void TransformController::setAxis(TransformAxis axis)
{
    m_Axis = axis;
}

TransformAxis TransformController::getAxis() const
{
    return m_Axis;
}

void TransformController::setSpace(TransformSpace space)
{
    m_Space = space;
}

TransformSpace TransformController::getSpace() const
{
    return m_Space;
}

void TransformController::applyPositiveStep()
{
    if (m_SelectedObject == nullptr || m_Mode != TransformMode::Translate)
    {
        return;
    }

    glm::vec3 position = m_SelectedObject->getTransform().getPosition();

    switch (m_Axis)
    {
    case TransformAxis::X:
        position.x += 0.1f;
        break;
    case TransformAxis::Y:
        position.y += 0.1f;
        break;
    case TransformAxis::Z:
        position.z += 0.1f;
        break;
    default:
        return;
    }

    m_SelectedObject->getTransform().setPosition(position);
}

void TransformController::applyNegativeStep()
{
    if (m_SelectedObject == nullptr || m_Mode != TransformMode::Translate)
    {
        return;
    }

    glm::vec3 position = m_SelectedObject->getTransform().getPosition();

    switch (m_Axis)
    {
    case TransformAxis::X:
        position.x -= 0.1f;
        break;
    case TransformAxis::Y:
        position.y -= 0.1f;
        break;
    case TransformAxis::Z:
        position.z -= 0.1f;
        break;
    default:
        return;
    }

    m_SelectedObject->getTransform().setPosition(position);
}

glm::vec3 TransformController::getAxisDirectionWorld() const
{
    if (m_Axis == TransformAxis::None)
    {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    glm::vec3 axisLocal(0.0f, 0.0f, 0.0f);

    switch (m_Axis)
    {
    case TransformAxis::X:
        axisLocal = glm::vec3(1.0f, 0.0f, 0.0f);
        break;
    case TransformAxis::Y:
        axisLocal = glm::vec3(0.0f, 1.0f, 0.0f);
        break;
    case TransformAxis::Z:
        axisLocal = glm::vec3(0.0f, 0.0f, 1.0f);
        break;
    default:
        break;
    }

    if (m_Space == TransformSpace::Global || m_SelectedObject == nullptr)
    {
        return axisLocal;
    }

    glm::mat4 model = m_SelectedObject->getTransform().getModelMatrix();
    glm::mat3 rotation(model);

    glm::vec3 axisWorld = rotation * axisLocal;

    if (glm::length(axisWorld) < 0.00001f)
    {
        return axisLocal;
    }

    return glm::normalize(axisWorld);
}

bool TransformController::intersectRayPlane(
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

void TransformController::beginDragFromRay(const Ray& ray)
{
    if (m_SelectedObject == nullptr)
    {
        return;
    }

    if (m_Mode != TransformMode::Translate)
    {
        return;
    }

    if (m_Axis == TransformAxis::None)
    {
        return;
    }

    glm::vec3 axisWorld = getAxisDirectionWorld();

    if (glm::length(axisWorld) < 0.00001f)
    {
        return;
    }

    glm::vec3 rayDir = glm::normalize(ray.direction);

    glm::vec3 helper = glm::cross(rayDir, axisWorld);

    if (glm::length(helper) < 0.00001f)
    {
        helper = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), axisWorld);

        if (glm::length(helper) < 0.00001f)
        {
            helper = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), axisWorld);
        }
    }

    glm::vec3 planeNormal = glm::cross(axisWorld, helper);

    if (glm::length(planeNormal) < 0.00001f)
    {
        return;
    }

    planeNormal = glm::normalize(planeNormal);

    glm::vec3 objectPosition = m_SelectedObject->getTransform().getPosition();
    glm::vec3 hitPoint(0.0f, 0.0f, 0.0f);

    if (!intersectRayPlane(ray, objectPosition, planeNormal, hitPoint))
    {
        return;
    }

    m_IsDragging = true;
    m_DragStartPosition = objectPosition;
    m_DragStartWorldPoint = hitPoint;
    m_DragAxisWorld = axisWorld;
    m_DragPlanePoint = objectPosition;
    m_DragPlaneNormal = planeNormal;

    std::cout << "[GIZMO] Drag iniciado\n";
}

void TransformController::updateDragFromRay(const Ray& ray)
{
    if (!m_IsDragging || m_SelectedObject == nullptr)
    {
        return;
    }

    glm::vec3 currentPoint(0.0f, 0.0f, 0.0f);

    if (!intersectRayPlane(ray, m_DragPlanePoint, m_DragPlaneNormal, currentPoint))
    {
        return;
    }

    glm::vec3 worldDelta = currentPoint - m_DragStartWorldPoint;
    float amount = glm::dot(worldDelta, m_DragAxisWorld);

    glm::vec3 newPosition = m_DragStartPosition + m_DragAxisWorld * amount;
    m_SelectedObject->getTransform().setPosition(newPosition);
}

void TransformController::endDrag()
{
    if (!m_IsDragging)
    {
        return;
    }

    m_IsDragging = false;
    std::cout << "[GIZMO] Drag finalizado\n";
}

bool TransformController::isDragging() const
{
    return m_IsDragging;
}