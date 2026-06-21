/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEM.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace locus::kernel::geometry {

    /**
     * @brief Low-level editor for LEM geometric mutations.
     *
     * GeometryEditor changes vertex positions and derived geometric data without
     * directly creating or deleting topology. It records accepted changes into the
     * shared LEMDiff owned by the parent LEMEditor facade.
     */
    class GeometryEditor {
    public:
        /**
         * @brief Creates a geometry editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives geometric mutations.
         * @param diff Diff that receives change events.
         */
        GeometryEditor(LEM& mesh, LEMDiff& diff);

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
         * @brief Sets a vertex position and updates adjacent face normals.
         *
         * @param handle Vertex to modify.
         * @param position New object-space position.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_vertex_position(VertexHandle handle, const glm::vec3& position);

        /**
         * @brief Translates a single vertex by an object-space offset.
         *
         * @param handle Vertex to translate.
         * @param offset Translation offset.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool translate_vertex(VertexHandle handle, const glm::vec3& offset);

        /**
         * @brief Translates multiple vertices by the same object-space offset.
         *
         * @param vertices Vertices to translate.
         * @param offset Translation offset.
         * @return Number of vertices accepted by the edit.
         */
        std::size_t translate_vertices(const std::vector<VertexHandle>& vertices, const glm::vec3& offset);

    private:
        void rebuild_adjacent_face_normals(VertexHandle vertex);

        LEM& mesh_;
        LEMDiff& diff_;
    };

}