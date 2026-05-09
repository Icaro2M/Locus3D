#include "ToolManager.h"

ToolManager::ToolManager(EditorState* state, AppEventBus* eventBus)
    : m_state(state), m_eventBus(eventBus)
{
}

void ToolManager::startTool(EditorToolType toolType)
{
    if (m_state->getActiveTool() != EditorToolType::None)
    {
        cancelCurrentTool();
    }

    if (!m_state->isFaceModeActive() || !m_state->getSelectedFace().isValid())
    {
        return;
    }

    m_state->setActiveTool(toolType);
    m_eventBus->emit(EventType::ToolStarted);
}

void ToolManager::cancelCurrentTool()
{
    if (m_state->getActiveTool() == EditorToolType::None)
    {
        return;
    }

    m_eventBus->emit(EventType::ToolCanceled);
}

bool ToolManager::confirmCurrentTool()
{
    if (m_state->getActiveTool() == EditorToolType::None)
    {
        return false;
    }

    m_eventBus->emit(EventType::ToolConfirmed);
    return true;
}

void ToolManager::handleInputEvent(EventType eventType)
{
    switch (eventType)
    {
    case EventType::InputKeyT:
        startTool(EditorToolType::PushPull);
        break;

    case EventType::InputKeyM:
        startTool(EditorToolType::FaceMove);
        break;

    case EventType::InputKeyS:
        if (m_state->isFaceModeActive())
        {
            startTool(EditorToolType::FaceScale);
        }
        break;

    case EventType::InputKeyEscape:
        cancelCurrentTool();
        break;

    case EventType::InputMouseClickLeft:
        confirmCurrentTool();
        break;

    default:
        break;
    }
}