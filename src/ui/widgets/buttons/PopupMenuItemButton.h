#pragma once

#include "ButtonTypes.h"

#include <string>

namespace ui
{
    struct PopupMenuItemButtonConfig
    {
        const char* id = nullptr;
        const char* label = nullptr;
        bool active = false;
        bool enabled = true;
        std::string iconPath;
        PopupMenuItemStyle style = PopupMenuItemStyle();
    };

    bool PopupMenuItemButton(const PopupMenuItemButtonConfig& config);
}