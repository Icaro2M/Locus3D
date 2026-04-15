#include "TransformController.h"

#include <algorithm>
#include <glm/glm/gtc/quaternion.hpp>

TransformController::TransformController()
    : m_SelectedObject(nullptr),
    m_Mode(TransformMode::None),
    m_Axis(TransformAxis::None),
    m_Space(TransformSpace::Global),
    m_DragStartPosition(0.0f, 0.0f, 0.0f),
    m_DragStartScale(1.0f, 1.0f, 1.0f)
{
}

void TransformController::setSelectedObject(SceneObject* object)
{
    if (m_SelectedObject == object)
    {
        return;
    }

    if (isDragging())
    {
        endDrag();
    }

    m_SelectedObject = object;
    m_Axis = TransformAxis::None;
}

SceneObject* TransformController::getSelectedObject() const
{
    return m_SelectedObject;
}

void TransformController::clearSelection()
{
    if (isDragging())
    {
        endDrag();
    }

    m_SelectedObject = nullptr;
    m_Axis = TransformAxis::None;
}

void TransformController::setMode(TransformMode mode)
{
    if (isDragging())
    {
        return;
    }

    if (m_Mode != mode)
    {
        m_Axis = TransformAxis::None;
    }

    m_Mode = mode;
}

TransformMode TransformController::getMode() const
{
    return m_Mode;
}

void TransformController::setAxis(TransformAxis axis)
{
    if (isDragging())
    {
        return;
    }

    m_Axis = axis;
}

TransformAxis TransformController::getAxis() const
{
    return m_Axis;
}

void TransformController::clearAxis()
{
    if (isDragging())
    {
        return;
    }

    m_Axis = TransformAxis::None;
}

void TransformController::setSpace(TransformSpace space)
{
    if (isDragging())
    {
        return;
    }

    m_Space = space;
}

TransformSpace TransformController::getSpace() const
{
    return m_Space;
}

bool TransformController::hasSelection() const
{
    return m_SelectedObject != nullptr;
}

bool TransformController::hasActiveMode() const
{
    return m_Mode != TransformMode::None;
}

glm::vec3 TransformController::buildAxisVector(TransformAxis axis) const
{
    switch (axis)
    {
    case TransformAxis::X:
        return glm::vec3(1.0f, 0.0f, 0.0f);
    case TransformAxis::Y:
        return glm::vec3(0.0f, 1.0f, 0.0f);
    case TransformAxis::Z:
        return glm::vec3(0.0f, 0.0f, 1.0f);
    default:
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

glm::vec3 TransformController::getAxisDirectionWorld() const
{
    glm::vec3 axisLocal = buildAxisVector(m_Axis);

    if (glm::length(axisLocal) < 0.00001f)
    {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    if (m_Space == TransformSpace::Global || m_SelectedObject == nullptr)
    {
        return axisLocal;
    }

    glm::quat orientation = m_SelectedObject->getTransform().getOrientation();
    glm::vec3 axisWorld = orientation * axisLocal;

    if (glm::length(axisWorld) < 0.00001f)
    {
        return axisLocal;
    }

    return glm::normalize(axisWorld);
}

bool TransformController::beginDragFromRay(const Ray& ray)
{
    if (m_SelectedObject == nullptr)
    {
        return false;
    }

    if (m_Mode == TransformMode::None || m_Mode == TransformMode::Rotate)
    {
        return false;
    }

    if (m_Axis == TransformAxis::None)
    {
        return false;
    }

    glm::vec3 axisWorld = getAxisDirectionWorld();

    if (glm::length(axisWorld) < 0.00001f)
    {
        return false;
    }

    m_DragStartPosition = m_SelectedObject->getTransform().getPosition();
    m_DragStartScale = m_SelectedObject->getTransform().getScale();

    return m_AxisDrag.begin(m_DragStartPosition, axisWorld, ray);
}

void TransformController::updateDragFromRay(const Ray& ray)
{
    if (m_SelectedObject == nullptr)
    {
        return;
    }

    if (!m_AxisDrag.isActive())
    {
        return;
    }

    m_AxisDrag.update(ray);

    float delta = m_AxisDrag.getCurrentDelta();
    glm::vec3 axisWorld = getAxisDirectionWorld();

    if (glm::length(axisWorld) < 0.00001f)
    {
        return;
    }

    if (m_Mode == TransformMode::Translate)
    {
        glm::vec3 newPosition = m_DragStartPosition + axisWorld * delta;
        m_SelectedObject->getTransform().setPosition(newPosition);
        return;
    }

    if (m_Mode == TransformMode::Scale)
    {
        glm::vec3 axisMask = buildAxisVector(m_Axis);
        float factor = std::max(0.05f, 1.0f + delta);

        glm::vec3 newScale = m_DragStartScale;

        if (axisMask.x > 0.5f)
        {
            newScale.x = std::max(0.05f, m_DragStartScale.x * factor);
        }

        if (axisMask.y > 0.5f)
        {
            newScale.y = std::max(0.05f, m_DragStartScale.y * factor);
        }

        if (axisMask.z > 0.5f)
        {
            newScale.z = std::max(0.05f, m_DragStartScale.z * factor);
        }

        m_SelectedObject->getTransform().setScale(newScale);
    }
}

void TransformController::endDrag()
{
    if (!m_AxisDrag.isActive())
    {
        return;
    }

    m_AxisDrag.reset();
}

bool TransformController::isDragging() const
{
    return m_AxisDrag.isActive();
}