#pragma once

#include "kernel/geometry/mesh/MeshHandles.h"

#include <glm/glm.hpp>

namespace locus::kernel::geometry
{
    struct Face
    {
        LoopHandle loop{};

        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };

        bool selected = false;
        bool hidden = false;
        bool deleted = false;
    };
}