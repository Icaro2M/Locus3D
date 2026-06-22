/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/geometry/GeometryPosition.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/editing/geometry/GeometryNormals.h"

#include <algorithm>
#include <glm/common.hpp>

namespace locus::kernel::geometry {

    GeometryPosition::GeometryPosition(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
    {
    }

    bool GeometryPosition::set_vertex_position(VertexHandle handle, const glm::vec3& position)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Vertex& vertex = mesh_.vertex(handle);
        if (vertex.position == position) {
            return true;
        }

        vertex.position = position;
        diff_.record(LEMChangeType::VertexModified, handle);

        GeometryNormals normals{ mesh_, diff_ };
        normals.rebuild_normals_around_vertex(handle);

        return true;
    }

    bool GeometryPosition::set_vertex_position_lerp(
        VertexHandle handle,
        const glm::vec3& target,
        float t)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        const float clampedT = std::clamp(t, 0.0f, 1.0f);
        const glm::vec3 current = mesh_.vertex(handle).position;
        return set_vertex_position(handle, glm::mix(current, target, clampedT));
    }

    bool GeometryPosition::translate_vertex(VertexHandle handle, const glm::vec3& offset)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        return set_vertex_position(handle, mesh_.vertex(handle).position + offset);
    }

    bool GeometryPosition::offset_vertex_along_normal(VertexHandle handle, float distance)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        GeometryNormals normals{ mesh_, diff_ };
        const glm::vec3 normal = normals.vertex_normal(handle);

        return translate_vertex(handle, normal * distance);
    }

}