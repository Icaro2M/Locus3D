#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    struct ViewportPalette
    {
        ColorRGBA background{ 0.08f, 0.08f, 0.09f, 1.0f };
        ColorRGBA gridMajor{ 0.38f, 0.38f, 0.40f, 1.0f };
        ColorRGBA gridMinor{ 0.20f, 0.20f, 0.22f, 1.0f };

        ColorRGBA objectDefault{ 0.85f, 0.85f, 0.88f, 1.0f };
        ColorRGBA objectSelected{ 1.0f, 0.68f, 0.18f, 1.0f };
        ColorRGBA objectHovered{ 0.45f, 0.72f, 1.0f, 1.0f };

        ColorRGBA wireframe{ 0.02f, 0.02f, 0.02f, 1.0f };
        ColorRGBA outline{ 1.0f, 0.72f, 0.20f, 1.0f };

        ColorRGBA axisX{ 0.90f, 0.20f, 0.20f, 1.0f };
        ColorRGBA axisY{ 0.20f, 0.80f, 0.25f, 1.0f };
        ColorRGBA axisZ{ 0.25f, 0.45f, 1.0f, 1.0f };
    };
}