#pragma once

#include "../CustomSolidPanelState.h"

#include <imgui.h>

class CustomSolidSidePreview
{
public:
    void draw(const CustomSolidPanelState& state, const ImVec2& size);

private:
    void drawFrame(const ImVec2& origin, const ImVec2& size);
    void drawSolid(const CustomSolidPanelState& state, const ImVec2& origin, const ImVec2& size);
};