#pragma once

#include <cstdint>

namespace locus::kernel::geometry
{
    using MeshIndex = std::uint32_t;
    using MeshElementCount = std::uint32_t;

    enum class MeshElementType
    {
        Vertex,
        Edge,
        Loop,
        Face
    };
}