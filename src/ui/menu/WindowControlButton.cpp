#include "WindowControlButton.h"

#include "../layout/UILayoutConstants.h"

#include <imgui.h>

namespace
{
    ImVec4 GetButtonColor(bool hovered, bool active, bool closeButton)
    {
        if (active)
        {
            return closeButton
                ? ImVec4(0.66f, 0.10f, 0.10f, 1.0f)
                : ImVec4(0.20f, 0.24f, 0.30f, 1.0f);
        }

        if (hovered)
        {
            return closeButton
                ? ImVec4(0.84f, 0.14f, 0.14f, 1.0f)
                : ImVec4(0.15f, 0.17f, 0.21f, 1.0f);
        }

        return ImVec4(0.060f, 0.064f, 0.076f, 1.0f);
    }

    void DrawMinimizeIcon(ImDrawList* drawList, ImVec2 center, ImU32 color, float thickness)
    {
        drawList->AddLine(
            ImVec2(center.x - 5.5f, center.y + 1.0f),
            ImVec2(center.x + 5.5f, center.y + 1.0f),
            color,
            thickness
        );
    }

    void DrawMaximizeIcon(ImDrawList* drawList, ImVec2 center, ImU32 color, float thickness)
    {
        drawList->AddRect(
            ImVec2(center.x - 5.0f, center.y - 5.0f),
            ImVec2(center.x + 5.0f, center.y + 5.0f),
            color,
            0.0f,
            0,
            thickness
        );
    }

    void DrawRestoreIcon(
        ImDrawList* drawList,
        ImVec2 center,
        ImU32 iconColor,
        ImU32 buttonColor,
        float thickness
    )
    {
        drawList->AddRect(
            ImVec2(center.x - 3.0f, center.y - 6.0f),
            ImVec2(center.x + 6.0f, center.y + 3.0f),
            iconColor,
            0.0f,
            0,
            thickness
        );
        drawList->AddRectFilled(
            ImVec2(center.x - 5.5f, center.y - 3.5f),
            ImVec2(center.x + 3.5f, center.y + 5.5f),
            buttonColor
        );
        drawList->AddRect(
            ImVec2(center.x - 6.0f, center.y - 3.0f),
            ImVec2(center.x + 3.0f, center.y + 6.0f),
            iconColor,
            0.0f,
            0,
            thickness
        );
    }

    void DrawCloseIcon(ImDrawList* drawList, ImVec2 center, ImU32 color, float thickness)
    {
        drawList->AddLine(
            ImVec2(center.x - 5.0f, center.y - 5.0f),
            ImVec2(center.x + 5.0f, center.y + 5.0f),
            color,
            thickness
        );
        drawList->AddLine(
            ImVec2(center.x + 5.0f, center.y - 5.0f),
            ImVec2(center.x - 5.0f, center.y + 5.0f),
            color,
            thickness
        );
    }
}

namespace ui::menu
{
    bool WindowControlButton(
        const char* id,
        WindowControlIcon icon,
        const char* tooltip,
        bool closeButton
    )
    {
        ImGui::PushID(id);

        const bool pressed = ImGui::InvisibleButton(
            id,
            ImVec2(WindowControlButtonWidth, ui::layout::TitleBarHeight)
        );

        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();

        const ImVec4 buttonColor = GetButtonColor(hovered, active, closeButton);
        const ImU32 buttonColorU32 = ImGui::GetColorU32(buttonColor);
        const ImU32 iconColor = ImGui::GetColorU32(
            hovered && closeButton
            ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
            : ImVec4(0.82f, 0.85f, 0.90f, 1.0f)
        );

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        const ImVec2 center(
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f
        );

        drawList->AddRectFilled(min, max, buttonColorU32);

        constexpr float thickness = 1.25f;

        switch (icon)
        {
        case WindowControlIcon::Minimize:
            DrawMinimizeIcon(drawList, center, iconColor, thickness);
            break;

        case WindowControlIcon::Maximize:
            DrawMaximizeIcon(drawList, center, iconColor, thickness);
            break;

        case WindowControlIcon::Restore:
            DrawRestoreIcon(drawList, center, iconColor, buttonColorU32, thickness);
            break;

        case WindowControlIcon::Close:
            DrawCloseIcon(drawList, center, iconColor, thickness);
            break;
        }

        if (hovered)
        {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();

        return pressed;
    }
}
