#include "TransformController.h"

#include <glm/glm/glm.hpp>

TransformController::TransformController()
    : m_SelectedObject(nullptr),
    m_Mode(TransformMode::None),
    m_Axis(TransformAxis::None),
    m_Space(TransformSpace::Global),
    m_DragStartPosition(0.0f, 0.0f, 0.0f)
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

    glm::vec3 objectPosition = m_SelectedObject->getTransform().getPosition();

    if (!m_AxisDrag.begin(objectPosition, axisWorld, ray))
    {
        return;
    }

    m_DragStartPosition = objectPosition;
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

    float amount = m_AxisDrag.getCurrentDelta();
    glm::vec3 axisWorld = getAxisDirectionWorld();

    if (glm::length(axisWorld) < 0.00001f)
    {
        return;
    }

    glm::vec3 newPosition = m_DragStartPosition + axisWorld * amount;
    m_SelectedObject->getTransform().setPosition(newPosition);
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