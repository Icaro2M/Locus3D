#pragma once

namespace ui::layout
{
    constexpr float TitleBarCompactHeight = 36.0f;
    constexpr float TitleBarComfortableHeight = 38.0f;
    constexpr float MainToolbarMinHeight = 52.0f;
    constexpr float MainToolbarMaxHeight = 58.0f;
    constexpr float ToolbarSpacing = 6.0f;

    constexpr float GizmoToolbarLeftMargin = 10.0f;

    constexpr float ViewportOverlayLeftMargin = 20.0f;
    constexpr float ViewportOverlayTopMargin = 16.0f;

    constexpr float SidePanelWidthRatio = 0.19f;
    constexpr float SidePanelMinWidth = 350.0f;
    constexpr float SidePanelMaxWidth = 650.0f;
    constexpr float SidePanelResizeHandleWidth = 7.0f;

    constexpr float SidePanelPaddingX = 12.0f;
    constexpr float SidePanelPaddingY = 10.0f;

    constexpr float SidePanelItemSpacingX = 8.0f;
    constexpr float SidePanelItemSpacingY = 7.0f;

    constexpr float InspectorWidthRatio = SidePanelWidthRatio;
    constexpr float InspectorMinWidth = SidePanelMinWidth;
    constexpr float InspectorMaxWidth = SidePanelMaxWidth;
    constexpr float InspectorResizeHandleWidth = SidePanelResizeHandleWidth;

    inline float GetTitleBarHeight(float viewportHeight)
    {
        return viewportHeight >= 1000.0f
            ? TitleBarCompactHeight
            : TitleBarComfortableHeight;
    }

    inline float GetMainToolbarHeight(float viewportHeight)
    {
        const float height = viewportHeight * 0.052f;

        if (height < MainToolbarMinHeight)
        {
            return MainToolbarMinHeight;
        }

        if (height > MainToolbarMaxHeight)
        {
            return MainToolbarMaxHeight;
        }

        return height;
    }
}
