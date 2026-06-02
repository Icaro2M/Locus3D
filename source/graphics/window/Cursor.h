#pragma once

namespace locus::graphics
{

    enum class CursorMode
    {
        Normal,
        Hidden,
        Disabled
    };

    enum class CursorShape
    {
        Arrow,
        IBeam,
        Crosshair,
        Hand,
        HorizontalResize,
        VerticalResize
    };

    struct CursorPosition
    {
        double x = 0.0;
        double y = 0.0;
    };

}