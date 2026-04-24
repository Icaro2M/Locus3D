#pragma once

#include "EditorState.h"
#include "AppEventBus.h"

class ToolManager {
public:
    ToolManager(EditorState* state, AppEventBus* eventBus);
    ~ToolManager() = default;

    void startTool(EditorToolType toolType);
    void cancelCurrentTool();
    bool confirmCurrentTool();

    void handleInputEvent(EventType eventType);

private:
    EditorState* m_state;
    AppEventBus* m_eventBus;
};