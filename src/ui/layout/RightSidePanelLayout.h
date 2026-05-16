#pragma once

#include <imgui.h>

struct UIContext;

namespace ui::layout
{
    struct RightSidePanelMetrics
    {
        ImVec2 position;
        ImVec2 size;
        float width = 0.0f;
        float height = 0.0f;
    };

    RightSidePanelMetrics CalculateRightSidePanelMetrics(UIContext* context);

    void DrawRightSidePanelResizeHandle(UIContext* context, const RightSidePanelMetrics& metrics);
}