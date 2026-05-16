#pragma once

#include "../CustomSolidPanelState.h"

#include <imgui.h>

class CustomSolidTopPreview
{
public:
    void draw(const CustomSolidPanelState& state, const ImVec2& size);

private:
    void drawFrame(const ImVec2& origin, const ImVec2& size);
    void drawPolygon(const CustomSolidPanelState& state, const ImVec2& origin, const ImVec2& size);
};