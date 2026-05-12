#include "DropdownButton.h"

namespace ui
{
    static ImVec4 ResolveBackgroundColor(const DropdownButtonConfig& config, bool hovered)
    {
        const ButtonVisualStyle& style = config.style;

        if (!config.enabled)
        {
            return style.disabledBackgroundColor;
        }

        if (config.open)
        {
            return style.activeColor;
        }

        if (hovered)
        {
            return style.hoverColor;
        }

        return style.backgroundColor;
    }

    static ImVec4 ResolveBorderColor(const DropdownButtonConfig& config)
    {
        const ButtonVisualStyle& style = config.style;

        if (!config.enabled)
        {
            return style.disabledBorderColor;
        }

        if (config.open)
        {
            return style.activeBorderColor;
        }

        return style.borderColor;
    }

    static ImVec4 ResolveIconColor(const DropdownButtonConfig& config)
    {
        const ButtonVisualStyle& style = config.style;

        if (!config.enabled)
        {
            return style.disabledIconColor;
        }

        if (config.open)
        {
            return style.activeIconColor;
        }

        return style.iconColor;
    }

    static void DrawArrowDown(
        ImDrawList* drawList,
        ImVec2 center,
        float size,
        ImU32 color
    )
    {
        ImVec2 p1 = ImVec2(center.x - size, center.y - size * 0.45f);
        ImVec2 p2 = ImVec2(center.x + size, center.y - size * 0.45f);
        ImVec2 p3 = ImVec2(center.x, center.y + size * 0.65f);

        drawList->AddTriangleFilled(p1, p2, p3, color);
    }

    bool DropdownButton(const DropdownButtonConfig& config)
    {
        if (config.id == nullptr)
        {
            return false;
        }

        ImGui::PushID(config.id);

        ButtonVisualStyle style = config.style;
        style.size = ImVec2(64.0f, style.size.y);

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = style.size;
        ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);

        bool pressed = false;

        if (config.enabled)
        {
            pressed = ImGui::InvisibleButton("##dropdown_button", size);
        }
        else
        {
            ImGui::BeginDisabled();
            ImGui::InvisibleButton("##dropdown_button", size);
            ImGui::EndDisabled();
        }

        bool hovered = config.enabled && ImGui::IsItemHovered();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec4 backgroundColor = ResolveBackgroundColor(config, hovered);
        ImVec4 borderColor = ResolveBorderColor(config);
        ImVec4 iconColor = ResolveIconColor(config);

        drawList->AddRectFilled(
            pos,
            max,
            ToU32(backgroundColor),
            style.rounding
        );

        drawList->AddRect(
            pos,
            max,
            ToU32(borderColor),
            style.rounding,
            0,
            style.borderThickness
        );

        if (config.iconDrawFn != nullptr)
        {
            ImVec2 iconCenter = ImVec2(
                pos.x + size.x * 0.38f,
                pos.y + size.y * 0.5f
            );

            float iconSize = size.y * style.iconScale;

            config.iconDrawFn(
                drawList,
                iconCenter,
                iconSize,
                ToU32(iconColor)
            );
        }

        ImVec2 arrowCenter = ImVec2(
            pos.x + size.x - 15.0f,
            pos.y + size.y * 0.5f + 1.0f
        );

        DrawArrowDown(
            drawList,
            arrowCenter,
            5.0f,
            ToU32(iconColor)
        );

        if (hovered && config.tooltip != nullptr && config.tooltip[0] != '\0')
        {
            ImGui::SetTooltip("%s", config.tooltip);
        }

        ImGui::PopID();

        return pressed && config.enabled;
    }
}   