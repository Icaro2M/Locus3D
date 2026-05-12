#include "PopupMenuItemButton.h"

namespace ui
{
    static ImVec4 ResolveBackgroundColor(const PopupMenuItemButtonConfig& config, bool hovered)
    {
        const PopupMenuItemStyle& style = config.style;

        if (config.active)
        {
            return style.activeColor;
        }

        if (hovered && config.enabled)
        {
            return style.hoverColor;
        }

        return style.backgroundColor;
    }

    static ImVec4 ResolveIconColor(const PopupMenuItemButtonConfig& config)
    {
        const PopupMenuItemStyle& style = config.style;

        if (!config.enabled)
        {
            return style.disabledIconColor;
        }

        return style.iconColor;
    }

    static ImVec4 ResolveTextColor(const PopupMenuItemButtonConfig& config)
    {
        const PopupMenuItemStyle& style = config.style;

        if (!config.enabled)
        {
            return style.disabledTextColor;
        }

        return style.textColor;
    }

    bool PopupMenuItemButton(const PopupMenuItemButtonConfig& config)
    {
        if (config.id == nullptr)
        {
            return false;
        }

        ImGui::PushID(config.id);

        const PopupMenuItemStyle& style = config.style;

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = style.size;
        ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);

        bool pressed = false;

        if (config.enabled)
        {
            pressed = ImGui::InvisibleButton("##popup_menu_item_button", size);
        }
        else
        {
            ImGui::BeginDisabled();
            ImGui::InvisibleButton("##popup_menu_item_button", size);
            ImGui::EndDisabled();
        }

        bool hovered = config.enabled && ImGui::IsItemHovered();

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec4 backgroundColor = ResolveBackgroundColor(config, hovered);
        ImVec4 iconColor = ResolveIconColor(config);
        ImVec4 textColor = ResolveTextColor(config);

        drawList->AddRectFilled(
            pos,
            max,
            ToU32(backgroundColor),
            style.rounding
        );

        if (config.iconDrawFn != nullptr)
        {
            ImVec2 iconCenter = ImVec2(
                pos.x + style.iconOffsetX + style.iconSize * 0.5f,
                pos.y + size.y * 0.5f
            );

            config.iconDrawFn(
                drawList,
                iconCenter,
                style.iconSize,
                ToU32(iconColor)
            );
        }

        if (config.label != nullptr)
        {
            ImVec2 textPos = ImVec2(
                pos.x + style.textOffsetX,
                pos.y + (size.y - ImGui::GetTextLineHeight()) * 0.5f
            );

            drawList->AddText(
                textPos,
                ToU32(textColor),
                config.label
            );
        }

        ImGui::PopID();

        return pressed && config.enabled;
    }
}