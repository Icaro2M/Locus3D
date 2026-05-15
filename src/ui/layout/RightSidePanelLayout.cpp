#include "RightSidePanelLayout.h"

#include "UILayoutConstants.h"
#include "../bridge/UIContext.h"

namespace
{
    float ClampPanelWidth(float width)
    {
        if (width < ui::layout::SidePanelMinWidth)
        {
            return ui::layout::SidePanelMinWidth;
        }

        if (width > ui::layout::SidePanelMaxWidth)
        {
            return ui::layout::SidePanelMaxWidth;
        }

        return width;
    }
}

namespace ui::layout
{
    RightSidePanelMetrics CalculateRightSidePanelMetrics(UIContext* context)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        if (!context->rightSidePanelHasUserResized)
        {
            context->rightSidePanelWidth = ClampPanelWidth(viewport->Size.x * SidePanelWidthRatio);
        }
        else
        {
            context->rightSidePanelWidth = ClampPanelWidth(context->rightSidePanelWidth);
        }

        const float startY = viewport->Pos.y + MainToolbarHeight;
        const float height = viewport->Size.y - MainToolbarHeight;

        RightSidePanelMetrics metrics;

        metrics.width = context->rightSidePanelWidth;
        metrics.height = height;
        metrics.position = ImVec2(viewport->Pos.x + viewport->Size.x - metrics.width, startY);
        metrics.size = ImVec2(metrics.width, height);

        return metrics;
    }

    void DrawRightSidePanelResizeHandle(UIContext* context, const RightSidePanelMetrics& metrics)
    {
        const float handleWidth = InspectorResizeHandleWidth;

        ImGui::SetCursorScreenPos(metrics.position);

        ImGui::InvisibleButton("##RightSidePanelResizeHandle", ImVec2(handleWidth, metrics.height));

        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();

        if (hovered || active)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        if (active)
        {
            context->rightSidePanelHasUserResized = true;
            context->rightSidePanelWidth = ClampPanelWidth(context->rightSidePanelWidth - ImGui::GetIO().MouseDelta.x);
        }

        if (hovered || active)
        {
            ImU32 color = ImGui::GetColorU32(
                active
                ? ImVec4(0.25f, 0.55f, 1.0f, 1.0f)
                : ImVec4(0.30f, 0.34f, 0.42f, 1.0f)
            );

            ImGui::GetWindowDrawList()->AddRectFilled(
                metrics.position,
                ImVec2(metrics.position.x + 2.0f, metrics.position.y + metrics.height),
                color
            );
        }
    }
}