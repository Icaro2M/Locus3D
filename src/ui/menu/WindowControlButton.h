#pragma once

namespace ui::menu
{
    enum class WindowControlIcon
    {
        Minimize,
        Maximize,
        Restore,
        Close
    };

    constexpr float WindowControlButtonWidth = 46.0f;

    bool WindowControlButton(
        const char* id,
        WindowControlIcon icon,
        const char* tooltip,
        bool closeButton = false
    );
}
