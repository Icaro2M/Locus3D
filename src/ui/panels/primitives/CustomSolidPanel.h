#pragma once

#include "../../UIContext.h"
#include "../../../application/AppEventBus.h"

class CustomSolidPanel {
public:
    CustomSolidPanel(AppEventBus* eventBus, UIContext* context);
    ~CustomSolidPanel() = default;

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;

    char m_nameBuffer[256];
    int m_sides;
    float m_bottomRadius;
    float m_topRadius;
    float m_height;
};