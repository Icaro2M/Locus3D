#pragma once

#include "kernel/geometry/mesh/MeshHandles.h"

namespace locus::kernel::geometry
{
    struct Loop
    {
        VertexHandle vertex{};
        EdgeHandle edge{};
        FaceHandle face{};

        LoopHandle next{};
        LoopHandle previous{};

        LoopHandle radialNext{};
        LoopHandle radialPrevious{};

        bool deleted = false;
    };
}