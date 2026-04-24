#include "TransformBridge.h"

TransformBridge::TransformBridge(EditorState* state, AppEventBus* eventBus)
    : m_state(state), m_eventBus(eventBus)
{
}

void TransformBridge::handleInputEvent(EventType eventType)
{
    if (m_state->getActiveTool() != EditorToolType::None || m_transformController.isDragging())
    {
        return;
    }

    switch (eventType)
    {
        case EventType::InputKeyW:
            m_state->setTransformMode(TransformMode::Translate);
            m_transformController.setMode(TransformMode::Translate);
            break;
        case EventType::InputKeyE:
            m_state->setTransformMode(TransformMode::Rotate);
            m_transformController.setMode(TransformMode::Rotate);
            break;
        case EventType::InputKeyR:
            m_state->setTransformMode(TransformMode::Scale);
            m_transformController.setMode(TransformMode::Scale);
            break;
        case EventType::InputKeyG:
            m_state->setTransformSpace(TransformSpace::Global);
            m_transformController.setSpace(TransformSpace::Global);
            break;
        case EventType::InputKeyL:
            m_state->setTransformSpace(TransformSpace::Local);
            m_transformController.setSpace(TransformSpace::Local);
            break;
        case EventType::InputKeyEscape:
            if (m_transformController.isDragging())
            {
                m_transformController.endDrag();
            }
            else
            {
                m_state->setTransformMode(TransformMode::None);
                m_transformController.setMode(TransformMode::None);
                m_transformController.clearAxis();
            }
            break;
        default:
            break;
    }
}

bool TransformBridge::handleMouseClick(GLFWwindow* window, Camera& camera, Raycaster& raycaster)
{
    SceneObject* selectedObject = m_state->getSelectedObject();
    
    if (m_state->getActiveTool() != EditorToolType::None || selectedObject == nullptr || m_state->getTransformMode() == TransformMode::None)
    {
        return false;
    }

    TransformAxis clickedAxis = TransformAxis::None;
    TransformMode currentMode = m_state->getTransformMode();
    TransformSpace currentSpace = m_state->getTransformSpace();

    m_transformController.setSelectedObject(selectedObject);

    if (currentMode == TransformMode::Translate)
    {
        clickedAxis = m_translateSelector.selectAxis(*selectedObject, window, camera, currentSpace);
    }
    else if (currentMode == TransformMode::Rotate)
    {
        clickedAxis = m_rotateSelector.selectAxis(*selectedObject, window, camera, currentSpace);
    }
    else if (currentMode == TransformMode::Scale)
    {
        clickedAxis = m_scaleSelector.selectAxis(*selectedObject, window, camera, currentSpace);
    }

    if (clickedAxis != TransformAxis::None)
    {
        m_state->setTransformAxis(clickedAxis);
        m_transformController.setAxis(clickedAxis);

        Ray clickRay = raycaster.buildRayFromMouse(window, camera);
        bool dragStarted = m_transformController.beginDragFromRay(clickRay);

        return dragStarted;
    }

    return false;
}

void TransformBridge::handleMouseMove(GLFWwindow* window, Camera& camera, Raycaster& raycaster)
{
    if (m_transformController.isDragging())
    {
        Ray dragRay = raycaster.buildRayFromMouse(window, camera);
        m_transformController.updateDragFromRay(dragRay);
        
        m_eventBus->emit(EventType::TransformChanged, 0);
    }
}

void TransformBridge::handleMouseRelease()
{
    if (m_transformController.isDragging())
    {
        m_transformController.endDrag();
    }
}

TransformController& TransformBridge::getController()
{
    return m_transformController;
}