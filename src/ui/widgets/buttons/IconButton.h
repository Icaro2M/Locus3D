#pragma once

#include "ButtonTypes.h"

namespace ui
{
    struct IconButtonConfig
    {
        const char* id = nullptr;
        const char* tooltip = nullptr;

        bool enabled = true;

        IconDrawFn iconDrawFn = nullptr;
        ButtonVisualStyle style = ButtonVisualStyle();
    };

    bool IconButton(const IconButtonConfig& config);
}