#include "ToolManager.h"

ToolManager::ToolManager(EditorState* state, AppEventBus* eventBus)
    : m_state(state), m_eventBus(eventBus)
{
}

void ToolManager::startTool(EditorToolType toolType)
{
    if (toolType == EditorToolType::None)
    {
        return;
    }

    if (m_state->getActiveTool() != EditorToolType::None)
    {
        m_eventBus->emit(EventType::ToolCanceled);
    }

    m_state->setFaceModeActive(true);
    m_state->clearSelectedFace();
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

    m_state->clearSelectedFace();
    m_state->setActiveTool(EditorToolType::None);
    m_state->setFaceModeActive(false);
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
        startTool(EditorToolType::FaceScale);
        break;

    case EventType::InputKeyEscape:
        cancelCurrentTool();
        break;

    default:
        break;
    }
}