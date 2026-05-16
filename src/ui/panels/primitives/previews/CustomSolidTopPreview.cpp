#include "CustomSolidTopPreview.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
    constexpr float Pi = 3.1415926535f;

    ImU32 FrameColor()
    {
        return IM_COL32(19, 22, 28, 255);
    }

    ImU32 BorderColor()
    {
        return IM_COL32(43, 48, 60, 255);
    }

    ImU32 MainLineColor()
    {
        return IM_COL32(215, 220, 230, 255);
    }

    ImU32 SecondaryLineColor()
    {
        return IM_COL32(100, 112, 135, 180);
    }

    float SafeMaxRadius(const CustomSolidPanelState& state)
    {
        return std::max(0.1f, std::max(state.bottomRadius, state.topRadius));
    }
}

void CustomSolidTopPreview::draw(const CustomSolidPanelState& state, const ImVec2& size)
{
    ImVec2 origin = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("##CustomSolidTopPreview", size);

    drawFrame(origin, size);
    drawPolygon(state, origin, size);
}

void CustomSolidTopPreview::drawFrame(const ImVec2& origin, const ImVec2& size)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(
        origin,
        ImVec2(origin.x + size.x, origin.y + size.y),
        FrameColor(),
        4.0f
    );

    drawList->AddRect(
        origin,
        ImVec2(origin.x + size.x, origin.y + size.y),
        BorderColor(),
        4.0f,
        0,
        1.0f
    );
}

void CustomSolidTopPreview::drawPolygon(const CustomSolidPanelState& state, const ImVec2& origin, const ImVec2& size)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImVec2 center(
        origin.x + size.x * 0.5f,
        origin.y + size.y * 0.5f
    );

    const int sides = std::clamp(state.sides, 3, 64);

    const float maxRadius = SafeMaxRadius(state);
    const float availableRadius = std::min(size.x, size.y) * 0.34f;

    const float bottomRadius = state.bottomRadius / maxRadius * availableRadius;
    const float topRadius = state.topRadius / maxRadius * availableRadius;

    std::vector<ImVec2> bottomPoints;
    std::vector<ImVec2> topPoints;

    bottomPoints.reserve(sides);
    topPoints.reserve(sides);

    for (int i = 0; i < sides; ++i)
    {
        const float angle = -Pi * 0.5f + static_cast<float>(i) * 2.0f * Pi / static_cast<float>(sides);
        const float x = std::cos(angle);
        const float y = std::sin(angle);

        bottomPoints.emplace_back(center.x + x * bottomRadius, center.y + y * bottomRadius);
        topPoints.emplace_back(center.x + x * topRadius, center.y + y * topRadius);
    }

    for (int i = 0; i < sides; ++i)
    {
        const int next = (i + 1) % sides;

        drawList->AddLine(bottomPoints[i], bottomPoints[next], MainLineColor(), 1.8f);

        if (std::abs(state.topRadius - state.bottomRadius) > 0.01f)
        {
            drawList->AddLine(topPoints[i], topPoints[next], SecondaryLineColor(), 1.2f);
        }
    }
}