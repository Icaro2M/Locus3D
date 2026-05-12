#pragma once

#include "../bridge/UIContext.h"
#include "../../application/AppEventBus.h"

class TransformPanel {
public:
    TransformPanel(AppEventBus* eventBus, UIContext* context);
    ~TransformPanel() = default;

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;
};