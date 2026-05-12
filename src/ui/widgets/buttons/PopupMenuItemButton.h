#pragma once

#include "ButtonTypes.h"

namespace ui
{
    struct PopupMenuItemButtonConfig
    {
        const char* id = nullptr;
        const char* label = nullptr;

        bool active = false;
        bool enabled = true;

        IconDrawFn iconDrawFn = nullptr;
        PopupMenuItemStyle style = PopupMenuItemStyle();
    };

    bool PopupMenuItemButton(const PopupMenuItemButtonConfig& config);
}