#pragma once

#include "ButtonTypes.h"

#include <string>

namespace ui
{
    struct IconButtonConfig
    {
        const char* id = nullptr;
        const char* tooltip = nullptr;
        bool enabled = true;
        std::string iconPath;
        ButtonVisualStyle style = ButtonVisualStyle();
    };

    bool IconButton(const IconButtonConfig& config);
}