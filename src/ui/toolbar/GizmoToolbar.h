#pragma once

#include "../../application/AppEventBus.h"
#include "../bridge/UIContext.h"

class GizmoToolbar
{
public:
    GizmoToolbar(AppEventBus* eventBus, UIContext* context);
    ~GizmoToolbar() = default;

    void draw();

private:
    void drawBackground();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;
};