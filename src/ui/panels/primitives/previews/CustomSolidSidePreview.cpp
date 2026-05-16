#include "CustomSolidSidePreview.h"

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

void CustomSolidSidePreview::draw(const CustomSolidPanelState& state, const ImVec2& size)
{
    ImVec2 origin = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("##CustomSolidSidePreview", size);

    drawFrame(origin, size);
    drawSolid(state, origin, size);
}

void CustomSolidSidePreview::drawFrame(const ImVec2& origin, const ImVec2& size)
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

void CustomSolidSidePreview::drawSolid(const CustomSolidPanelState& state, const ImVec2& origin, const ImVec2& size)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImVec2 center(
        origin.x + size.x * 0.5f,
        origin.y + size.y * 0.52f
    );

    const int sides = std::clamp(state.sides, 3, 64);

    const float maxRadius = SafeMaxRadius(state);
    const float safeHeight = std::max(0.1f, state.height);

    const float availableWidth = size.x * 0.66f;
    const float availableHeight = size.y * 0.62f;

    const float scale = std::min(
        availableWidth / (maxRadius * 2.0f),
        availableHeight / safeHeight
    );

    const float bottomRadius = state.bottomRadius * scale;
    const float topRadius = state.topRadius * scale;
    const float height = state.height * scale;

    const float yTop = center.y - height * 0.5f;
    const float yBottom = center.y + height * 0.5f;

    constexpr float tilt = 0.35f;

    std::vector<ImVec2> topPoints;
    std::vector<ImVec2> bottomPoints;

    topPoints.reserve(sides);
    bottomPoints.reserve(sides);

    for (int i = 0; i < sides; ++i)
    {
        const float angle = -Pi * 0.5f + static_cast<float>(i) * 2.0f * Pi / static_cast<float>(sides);
        const float x = std::cos(angle);
        const float z = std::sin(angle);

        topPoints.emplace_back(center.x + x * topRadius, yTop + z * topRadius * tilt);
        bottomPoints.emplace_back(center.x + x * bottomRadius, yBottom + z * bottomRadius * tilt);
    }

    for (int i = 0; i < sides; ++i)
    {
        const int next = (i + 1) % sides;

        drawList->AddLine(bottomPoints[i], bottomPoints[next], SecondaryLineColor(), 1.2f);
        drawList->AddLine(topPoints[i], topPoints[next], MainLineColor(), 1.8f);
        drawList->AddLine(topPoints[i], bottomPoints[i], MainLineColor(), 1.4f);
    }
}