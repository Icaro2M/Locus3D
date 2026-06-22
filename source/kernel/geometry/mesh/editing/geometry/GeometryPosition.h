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
     * @brief Edits vertex positions on a Locus Editable Mesh.
     */
    class GeometryPosition {
    public:
        /**
         * @brief Creates a position editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives position edits.
         * @param diff Diff that receives vertex and normal change events.
         */
        GeometryPosition(LEM& mesh, LEMDiff& diff);

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
         * @brief Translates one vertex by an object-space offset.
         *
         * @param handle Vertex to translate.
         * @param offset Translation offset.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool translate_vertex(VertexHandle handle, const glm::vec3& offset);

        /**
         * @brief Moves a vertex along the averaged normal of adjacent faces.
         *
         * @param handle Vertex to move.
         * @param distance Signed movement distance.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool offset_vertex_along_normal(VertexHandle handle, float distance);

    private:
        LEM& mesh_;
        LEMDiff& diff_;
    };

}