#pragma once

#include <imgui.h>

namespace ui::fonts
{
    void LoadFonts(ImGuiIO& io);

    ImFont* Default();
    ImFont* TopBar(float viewportHeight);
    float TopBarSize(float viewportHeight);
}
