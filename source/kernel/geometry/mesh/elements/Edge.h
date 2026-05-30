#pragma once

#include "kernel/geometry/mesh/MeshHandles.h"

namespace locus::kernel::geometry
{
    struct Edge
    {
        VertexHandle vertexA{};
        VertexHandle vertexB{};

        LoopHandle loop{};

        bool selected = false;
        bool hidden = false;
        bool deleted = false;
    };
}