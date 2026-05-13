#pragma once

#include "../bridge/UIContext.h"
#include "../../application/AppEventBus.h"

class GizmoToolbar
{
public:
    GizmoToolbar(AppEventBus* eventBus, UIContext* context);
    ~GizmoToolbar() = default;

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;
};