/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/geometry/GeometryTransform.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/editing/geometry/GeometryPosition.h"

#include <glm/vec4.hpp>

namespace locus::kernel::geometry {

    GeometryTransform::GeometryTransform(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
    {
    }

    std::size_t GeometryTransform::translate_vertices(
        const std::vector<VertexHandle>& vertices,
        const glm::vec3& offset)
    {
        std::size_t count = 0;
        GeometryPosition position{ mesh_, diff_ };

        for (VertexHandle handle : vertices) {
            if (position.translate_vertex(handle, offset)) {
                ++count;
            }
        }

        return count;
    }

    std::size_t GeometryTransform::transform_vertices(
        const std::vector<VertexHandle>& vertices,
        const glm::mat4& transform)
    {
        std::size_t count = 0;
        GeometryPosition position{ mesh_, diff_ };

        for (VertexHandle handle : vertices) {
            if (!mesh_.is_valid(handle)) {
                continue;
            }

            const glm::vec4 transformed = transform * glm::vec4(mesh_.vertex(handle).position, 1.0f);
            if (position.set_vertex_position(handle, glm::vec3(transformed))) {
                ++count;
            }
        }

        return count;
    }

}