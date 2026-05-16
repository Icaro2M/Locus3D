#pragma once

#include "ButtonTypes.h"

#include <string>

namespace ui
{
    struct DropdownButtonConfig
    {
        const char* id = nullptr;
        const char* tooltip = nullptr;
        bool open = false;
        bool enabled = true;
        std::string iconPath;
        ButtonVisualStyle style = ButtonVisualStyle();
    };

    bool DropdownButton(const DropdownButtonConfig& config);
}