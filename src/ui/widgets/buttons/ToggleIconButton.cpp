#include "ToggleIconButton.h"

#include "../../icons/IconTextureCache.h"

namespace ui
{
    static ImVec4 ResolveBackgroundColor(const ToggleIconButtonConfig& config, bool hovered)
    {
        const ButtonVisualStyle& style = config.style;

        if (!config.enabled)
        {
            return style.disabledBackgroundColor;
        }

        if (config.active && hovered)
        {
            return style.activeHoverColor;
        }

        if (config.active)
        {
            return style.activeColor;
        }

        if (hovered)
        {
            return style.hoverColor;
        }

        return style.backgroundColor;
    }

    static ImVec4 ResolveBorderColor(const ToggleIconButtonConfig& config)
    {
        const ButtonVisualStyle& style = config.style;

        if (!config.enabled)
        {
            return style.disabledBorderColor;
        }

        if (config.active)
        {
            return style.activeBorderColor;
        }

        return style.borderColor;
    }

    static ImVec4 ResolveIconColor(const ToggleIconButtonConfig& config)
    {
        const ButtonVisualStyle& style = config.style;

        if (!config.enabled)
        {
            return style.disabledIconColor;
        }

        if (config.active)
        {
            return style.activeIconColor;
        }

        return style.iconColor;
    }

    static void DrawIconTexture(
        const std::string& iconPath,
        const ImVec2& center,
        float iconSize,
        const ImVec4& color
    )
    {
        if (iconPath.empty())
        {
            return;
        }

        ImTextureID textureId = IconTextureCache::Get(iconPath);

        if (textureId == 0)
        {
            return;
        }

        ImVec2 iconMin = ImVec2(
            center.x - iconSize * 0.5f,
            center.y - iconSize * 0.5f
        );

        ImVec2 iconMax = ImVec2(
            center.x + iconSize * 0.5f,
            center.y + iconSize * 0.5f
        );

        ImGui::GetWindowDrawList()->AddImage(
            textureId,
            iconMin,
            iconMax,
            ImVec2(0.0f, 0.0f),
            ImVec2(1.0f, 1.0f),
            ToU32(color)
        );
    }

    bool ToggleIconButton(const ToggleIconButtonConfig& config)
    {
        if (config.id == nullptr)
        {
            return false;
        }

        ImGui::PushID(config.id);

        const ButtonVisualStyle& style = config.style;

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = style.size;
        ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);

        bool pressed = false;

        if (config.enabled)
        {
            pressed = ImGui::InvisibleButton("##toggle_icon_button", size);
        }
        else
        {
            ImGui::BeginDisabled();
            ImGui::InvisibleButton("##toggle_icon_button", size);
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

        ImVec2 center = ImVec2(
            pos.x + size.x * 0.5f,
            pos.y + size.y * 0.5f
        );

        float iconSize = size.x < size.y ? size.x : size.y;
        iconSize *= style.iconScale;

        DrawIconTexture(config.iconPath, center, iconSize, iconColor);

        if (hovered && config.tooltip != nullptr && config.tooltip[0] != '\0')
        {
            ImGui::SetTooltip("%s", config.tooltip);
        }

        ImGui::PopID();

        return pressed && config.enabled;
    }
}