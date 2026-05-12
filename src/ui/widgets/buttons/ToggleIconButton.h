#pragma once

#include "ButtonTypes.h"

namespace ui
{
    struct ToggleIconButtonConfig
    {
        const char* id = nullptr;
        const char* tooltip = nullptr;

        bool active = false;
        bool enabled = true;

        IconDrawFn iconDrawFn = nullptr;
        ButtonVisualStyle style = ButtonVisualStyle();
    };

    bool ToggleIconButton(const ToggleIconButtonConfig& config);
}