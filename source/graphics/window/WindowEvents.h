#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{

    struct WindowResizeEvent
    {
        i32 width = 0;
        i32 height = 0;
    };

    struct FramebufferResizeEvent
    {
        i32 width = 0;
        i32 height = 0;
    };

    struct WindowFocusEvent
    {
        bool focused = false;
    };

    struct WindowCloseEvent
    {
    };

    struct CursorMoveEvent
    {
        double x = 0.0;
        double y = 0.0;
    };

    struct MouseButtonEvent
    {
        i32 button = 0;
        i32 action = 0;
        i32 mods = 0;
    };

    struct ScrollEvent
    {
        double xOffset = 0.0;
        double yOffset = 0.0;
    };

    struct KeyEvent
    {
        i32 key = 0;
        i32 scancode = 0;
        i32 action = 0;
        i32 mods = 0;
    };

}