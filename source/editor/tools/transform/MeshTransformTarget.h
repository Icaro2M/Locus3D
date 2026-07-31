/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/vec3.hpp>

#include <vector>

namespace locus::editor {

    /**
     * @brief Resolved mesh-component transform target.
     *
     * The target stores only the mesh node, the unique affected vertices, and the
     * world-space pivot used by the shared transform gizmo.
     */
    struct MeshTransformTarget {
        SceneNodeId node{};
        std::vector<kernel::geometry::VertexHandle> vertices{};
        glm::vec3 pivot{ 0.0f, 0.0f, 0.0f };

        [[nodiscard]] bool is_valid() const
        {
            return node.is_valid() && !vertices.empty();
        }
    };

} // namespace locus::editor
