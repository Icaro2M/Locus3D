/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/mesh/editing/geometry/GeometryNormals.h"
#include "kernel/geometry/mesh/editing/geometry/GeometryPosition.h"
#include "kernel/geometry/mesh/editing/geometry/GeometryTransform.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <vector>

namespace locus::kernel::geometry {

    class LEM;

    /**
     * @brief Facade for geometric mutations on a Locus Editable Mesh.
     *
     * GeometryEditor exposes a stable public access point for operations that
     * edit vertex positions, transform vertices, and rebuild derived normals.
     * The implementation is delegated to smaller internal geometry modules
     * under editing/geometry.
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
         * @brief Linearly interpolates a vertex position toward a target.
         *
         * @param handle Vertex to modify.
         * @param target Target object-space position.
         * @param t Interpolation factor clamped to the range [0, 1].
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_vertex_position_lerp(VertexHandle handle, const glm::vec3& target, float t);

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

        /**
         * @brief Applies a transform matrix to multiple vertices.
         *
         * @param vertices Vertices to transform.
         * @param transform Transform matrix applied to each position.
         * @return Number of vertices accepted by the edit.
         */
        std::size_t transform_vertices(const std::vector<VertexHandle>& vertices, const glm::mat4& transform);

        /**
         * @brief Moves a vertex along the averaged normal of adjacent faces.
         *
         * @param handle Vertex to move.
         * @param distance Signed movement distance.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool offset_vertex_along_normal(VertexHandle handle, float distance);

        /**
         * @brief Recomputes all active face normals.
         */
        void rebuild_face_normals();

        /**
         * @brief Recomputes normals for faces adjacent to a vertex.
         *
         * @param vertex Vertex whose adjacent faces will be updated.
         */
        void rebuild_normals_around_vertex(VertexHandle vertex);

        /**
         * @brief Recomputes a single face normal.
         *
         * @param face Face whose normal will be updated.
         * @return True when the face exists and was updated.
         */
        bool rebuild_normals_around_face(FaceHandle face);

    private:
        LEM& mesh_;
        LEMDiff& diff_;
        GeometryPosition position_;
        GeometryTransform transform_;
        GeometryNormals normals_;
    };

}