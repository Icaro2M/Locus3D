#pragma once

#include "../bridge/UIContext.h"
#include "../../application/AppEventBus.h"

class InspectorPanel {
public:
    InspectorPanel(AppEventBus* eventBus, UIContext* context);
    ~InspectorPanel() = default;

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;
    
    char m_renameBuffer[256];
};