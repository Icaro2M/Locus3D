#pragma once

#include "ButtonTypes.h"

namespace ui
{
    struct DropdownButtonConfig
    {
        const char* id = nullptr;
        const char* tooltip = nullptr;

        bool open = false;
        bool enabled = true;

        IconDrawFn iconDrawFn = nullptr;
        ButtonVisualStyle style = ButtonVisualStyle();
    };

    bool DropdownButton(const DropdownButtonConfig& config);
}