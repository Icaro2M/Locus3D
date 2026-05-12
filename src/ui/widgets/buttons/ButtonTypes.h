#pragma once

#include <imgui.h>

namespace ui
{
    using IconDrawFn = void (*)(
        ImDrawList* drawList,
        ImVec2 center,
        float size,
        ImU32 color
        );

    struct ButtonVisualStyle
    {
        ImVec2 size = ImVec2(38.0f, 38.0f);

        float rounding = 9.0f;
        float borderThickness = 1.0f;
        float iconScale = 0.56f;

        ImVec4 backgroundColor = ImVec4(0.10f, 0.11f, 0.13f, 1.0f);
        ImVec4 hoverColor = ImVec4(0.15f, 0.17f, 0.20f, 1.0f);
        ImVec4 activeColor = ImVec4(0.05f, 0.36f, 0.78f, 1.0f);
        ImVec4 activeHoverColor = ImVec4(0.07f, 0.43f, 0.92f, 1.0f);

        ImVec4 borderColor = ImVec4(0.24f, 0.26f, 0.30f, 1.0f);
        ImVec4 activeBorderColor = ImVec4(0.25f, 0.55f, 1.0f, 1.0f);

        ImVec4 iconColor = ImVec4(0.72f, 0.75f, 0.80f, 1.0f);
        ImVec4 activeIconColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        ImVec4 disabledBackgroundColor = ImVec4(0.08f, 0.09f, 0.10f, 1.0f);
        ImVec4 disabledBorderColor = ImVec4(0.16f, 0.17f, 0.19f, 1.0f);
        ImVec4 disabledIconColor = ImVec4(0.38f, 0.40f, 0.44f, 1.0f);
    };

    struct PopupMenuItemStyle
    {
        ImVec2 size = ImVec2(190.0f, 34.0f);

        float rounding = 6.0f;
        float iconSize = 18.0f;
        float iconOffsetX = 14.0f;
        float textOffsetX = 44.0f;

        ImVec4 backgroundColor = ImVec4(0.07f, 0.08f, 0.10f, 1.0f);
        ImVec4 hoverColor = ImVec4(0.13f, 0.15f, 0.18f, 1.0f);
        ImVec4 activeColor = ImVec4(0.05f, 0.36f, 0.78f, 1.0f);

        ImVec4 iconColor = ImVec4(0.88f, 0.90f, 0.94f, 1.0f);
        ImVec4 textColor = ImVec4(0.92f, 0.94f, 0.98f, 1.0f);
        ImVec4 disabledIconColor = ImVec4(0.38f, 0.40f, 0.44f, 1.0f);
        ImVec4 disabledTextColor = ImVec4(0.48f, 0.50f, 0.54f, 1.0f);
    };

    inline ImU32 ToU32(const ImVec4& color)
    {
        return ImGui::ColorConvertFloat4ToU32(color);
    }
}