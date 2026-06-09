#pragma once

#include "graphics/common/GraphicsTypes.h"
#include "graphics/scene/RenderLayer.h"

namespace locus::graphics
{
    struct RenderObject;

    struct RenderCommand
    {
        const RenderObject* object = nullptr;
        RenderLayer layer = RenderLayer::Default;
        u32 priority = 0;
        u32 sequence = 0;
    };
}