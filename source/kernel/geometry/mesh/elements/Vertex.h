#pragma once

#include "kernel/geometry/mesh/MeshHandles.h"

#include <glm/glm.hpp>

namespace locus::kernel::geometry
{
    struct Vertex
    {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        EdgeHandle edge{};

        bool selected = false;
        bool hidden = false;
        bool deleted = false;
    };
}