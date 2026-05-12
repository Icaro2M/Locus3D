#pragma once

#include "ButtonTypes.h"

#include <string>

namespace ui
{
    struct ToggleIconButtonConfig
    {
        const char* id = nullptr;
        const char* tooltip = nullptr;
        bool active = false;
        bool enabled = true;
        std::string iconPath;
        ButtonVisualStyle style = ButtonVisualStyle();
    };

    bool ToggleIconButton(const ToggleIconButtonConfig& config);
}