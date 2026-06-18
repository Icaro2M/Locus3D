#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/glm.hpp>

#include <vector>

namespace locus::kernel::geometry {

struct PrimitiveBuildResult {
    std::vector<VertexHandle> vertices{};
    std::vector<EdgeHandle> edges{};
    std::vector<FaceHandle> faces{};
    LEMDiff diff{};
    bool success = false;

    [[nodiscard]] explicit operator bool() const
    {
        return success;
    }

    [[nodiscard]] bool empty() const
    {
        return vertices.empty() && edges.empty() && faces.empty();
    }
};

struct BoxParameters {
    glm::vec3 center{ 0.0f, 0.0f, 0.0f };
    glm::vec3 size{ 1.0f, 1.0f, 1.0f };
    bool selectCreatedFaces = false;

    [[nodiscard]] bool is_valid() const
    {
        return size.x > 0.0f && size.y > 0.0f && size.z > 0.0f;
    }
};

}