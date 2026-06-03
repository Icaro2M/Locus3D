#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{

    struct MeshDrawData
    {
        PrimitiveTopology topology = PrimitiveTopology::Triangles;
        IndexType indexType = IndexType::UInt32;

        bool indexed = false;

        u32 vertexCount = 0;
        u32 indexCount = 0;
    };

}