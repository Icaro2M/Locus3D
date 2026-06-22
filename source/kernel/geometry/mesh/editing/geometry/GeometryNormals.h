/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/vec3.hpp>

namespace locus::kernel::geometry {

    class LEM;

    /**
     * @brief Rebuilds derived normal data stored by editable mesh faces.
     */
    class GeometryNormals {
    public:
        /**
         * @brief Creates a normal editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that owns the normals.
         * @param diff Diff that receives normal change events.
         */
        GeometryNormals(LEM& mesh, LEMDiff& diff);

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
         * @brief Recomputes one active face normal.
         *
         * @param face Face whose normal will be updated.
         * @return True when the face exists and was updated.
         */
        bool rebuild_normals_around_face(FaceHandle face);

        /**
         * @brief Computes an averaged normal from faces adjacent to a vertex.
         *
         * @param vertex Vertex whose adjacent face normals will be averaged.
         * @return Unit normal when possible, or the default up normal.
         */
        [[nodiscard]] glm::vec3 vertex_normal(VertexHandle vertex) const;

    private:
        LEM& mesh_;
        LEMDiff& diff_;
    };

}