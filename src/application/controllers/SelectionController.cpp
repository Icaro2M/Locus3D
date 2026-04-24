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
        SceneObject* selectedObj = m_state->getSelectedObject();
        int faceIndex = m_faceSelector.selectFace(*selectedObj, window, camera);

        if (faceIndex != -1)
        {
            m_state->getSelectedFace().set(selectedObj, faceIndex);
        }
        else
        {
            m_state->clearSelectedFace();
        }
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