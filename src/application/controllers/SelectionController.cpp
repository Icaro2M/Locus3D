#include "SelectionController.h"

SelectionController::SelectionController(EditorState* state, AppEventBus* eventBus)
    : m_state(state), m_eventBus(eventBus)
{
}

void SelectionController::handleSelection(GLFWwindow* window, Camera& camera, Scene& scene)
{
    if (m_state->getActiveTool() != EditorToolType::None)
    {
        return;
    }

    if (m_state->isFaceModeActive() && m_state->getSelectedObject() != nullptr)
    {
        selectFaceUnderMouse(window, camera);
    }
    else if (!m_state->isFaceModeActive())
    {
        SceneObject* hitObject = m_objectSelector.selectObject(window, camera, scene);

        if (hitObject != m_state->getSelectedObject())
        {
            m_state->setSelectedObject(hitObject);
        }
    }
}

bool SelectionController::selectFaceUnderMouse(GLFWwindow* window, Camera& camera)
{
    SceneObject* selectedObj = m_state->getSelectedObject();

    if (selectedObj == nullptr)
    {
        m_state->clearSelectedFace();
        return false;
    }

    int faceIndex = m_faceSelector.selectFace(*selectedObj, window, camera);

    if (faceIndex == -1)
    {
        m_state->clearSelectedFace();
        return false;
    }

    m_state->getSelectedFace().set(selectedObj, faceIndex);
    return true;
}

bool SelectionController::updateHoveredFace(GLFWwindow* window, Camera& camera)
{
    SceneObject* selectedObj = m_state->getSelectedObject();

    if (selectedObj == nullptr)
    {
        m_state->clearHoveredFace();
        return false;
    }

    if (m_state->getActiveTool() == EditorToolType::None)
    {
        m_state->clearHoveredFace();
        return false;
    }

    int faceIndex = m_faceSelector.selectFace(*selectedObj, window, camera);

    if (faceIndex == -1)
    {
        m_state->clearHoveredFace();
        return false;
    }

    m_state->getHoveredFace().set(selectedObj, faceIndex);
    return true;
}