#include "TransformController.h"

#include <algorithm>

TransformController::TransformController()
    : m_SelectedObject(nullptr),
    m_Mode(TransformMode::None),
    m_Axis(TransformAxis::None),
    m_Space(TransformSpace::Global),
    m_DragStartPosition(0.0f, 0.0f, 0.0f),
    m_DragStartScale(1.0f, 1.0f, 1.0f),
    m_DragStartRotation(0.0f, 0.0f, 0.0f)
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

    glm::mat4 model = m_SelectedObject->getTransform().getModelMatrix();
    glm::mat3 rotation(model);

    glm::vec3 axisWorld = rotation * axisLocal;

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
    m_DragStartRotation = m_SelectedObject->getTransform().getRotation();

    if (m_Mode == TransformMode::Translate)
    {
        return m_AxisDrag.begin(m_DragStartPosition, axisWorld, ray);
    }

    if (m_Mode == TransformMode::Scale)
    {
        return m_AxisDrag.begin(m_DragStartPosition, axisWorld, ray);
    }

    if (m_Mode == TransformMode::Rotate)
    {
        return m_RotateDrag.begin(m_DragStartPosition, axisWorld, ray);
    }

    return false;
}

void TransformController::updateDragFromRay(const Ray& ray)
{
    if (m_SelectedObject == nullptr)
    {
        return;
    }

    glm::vec3 axisWorld = getAxisDirectionWorld();

    if (glm::length(axisWorld) < 0.00001f)
    {
        return;
    }

    if (m_Mode == TransformMode::Translate && m_AxisDrag.isActive())
    {
        m_AxisDrag.update(ray);

        float delta = m_AxisDrag.getCurrentDelta();
        glm::vec3 newPosition = m_DragStartPosition + axisWorld * delta;
        m_SelectedObject->getTransform().setPosition(newPosition);
        return;
    }

    if (m_Mode == TransformMode::Scale && m_AxisDrag.isActive())
    {
        m_AxisDrag.update(ray);

        float delta = m_AxisDrag.getCurrentDelta();
        float factor = std::max(0.05f, 1.0f + delta);

        glm::vec3 newScale = m_DragStartScale;

        if (m_Axis == TransformAxis::X)
        {
            newScale.x = std::max(0.05f, m_DragStartScale.x * factor);
        }
        else if (m_Axis == TransformAxis::Y)
        {
            newScale.y = std::max(0.05f, m_DragStartScale.y * factor);
        }
        else if (m_Axis == TransformAxis::Z)
        {
            newScale.z = std::max(0.05f, m_DragStartScale.z * factor);
        }

        m_SelectedObject->getTransform().setScale(newScale);
        return;
    }

    if (m_Mode == TransformMode::Rotate && m_RotateDrag.isActive())
    {
        m_RotateDrag.update(ray);

        float deltaAngle = m_RotateDrag.getCurrentAngleDegrees();
        glm::vec3 newRotation = m_DragStartRotation;

        if (m_Axis == TransformAxis::X)
        {
            newRotation.x = m_DragStartRotation.x + deltaAngle;
        }
        else if (m_Axis == TransformAxis::Y)
        {
            newRotation.y = m_DragStartRotation.y + deltaAngle;
        }
        else if (m_Axis == TransformAxis::Z)
        {
            newRotation.z = m_DragStartRotation.z + deltaAngle;
        }

        m_SelectedObject->getTransform().setRotation(newRotation);
    }
}

void TransformController::endDrag()
{
    if (m_AxisDrag.isActive())
    {
        m_AxisDrag.reset();
    }

    if (m_RotateDrag.isActive())
    {
        m_RotateDrag.reset();
    }
}

bool TransformController::isDragging() const
{
    return m_AxisDrag.isActive() || m_RotateDrag.isActive();
}