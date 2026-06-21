/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEM.h"

#include <glm/glm.hpp>

#include <vector>

namespace locus::kernel::geometry {

    /**
     * @brief Low-level editor for LEM topology mutations.
     *
     * TopologyEditor owns editing operations that create, remove, or rebuild
     * topological mesh structure. It records all accepted topology changes into
     * the shared LEMDiff owned by the parent LEMEditor facade.
     */
    class TopologyEditor {
    public:
        /**
         * @brief Creates a topology editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives topology mutations.
         * @param diff Diff that receives change events.
         */
        TopologyEditor(LEM& mesh, LEMDiff& diff);

        /**
         * @brief Returns the edited mesh.
         *
         * @return Mutable mesh reference.
         */
        [[nodiscard]] LEM& mesh();

        /**
         * @brief Returns the edited mesh.
         *
         * @return Read-only mesh reference.
         */
        [[nodiscard]] const LEM& mesh() const;

        /**
         * @brief Adds a loose vertex and records the created element.
         *
         * @param position Vertex position in object space.
         * @return Handle referencing the created vertex.
         */
        VertexHandle add_vertex(const glm::vec3& position);

        /**
         * @brief Finds or creates a non-directional edge and records insertions.
         *
         * @param vertexA First endpoint vertex.
         * @param vertexB Second endpoint vertex.
         * @return Handle referencing the existing or created edge.
         */
        EdgeHandle find_or_create_edge(VertexHandle vertexA, VertexHandle vertexB);

        /**
         * @brief Adds a polygonal face and records all created topology elements.
         *
         * @param vertices Ordered face vertices.
         * @return Handle referencing the created face, or an invalid handle on failure.
         */
        FaceHandle add_face(const std::vector<VertexHandle>& vertices);

        /**
         * @brief Recomputes all active face normals and records normal changes.
         */
        void rebuild_face_normals();

        /**
         * @brief Clears the whole mesh and records a mesh clear event.
         */
        void clear();

    private:
        LEM& mesh_;
        LEMDiff& diff_;
    };

}