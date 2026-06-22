/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <vector>

namespace locus::kernel::geometry {

    class LEM;

    /**
     * @brief Applies object-space transformations to editable mesh vertices.
     */
    class GeometryTransform {
    public:
        /**
         * @brief Creates a transform editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives transform edits.
         * @param diff Diff that receives vertex and normal change events.
         */
        GeometryTransform(LEM& mesh, LEMDiff& diff);

        /**
         * @brief Translates multiple vertices by the same object-space offset.
         *
         * @param vertices Vertices to translate.
         * @param offset Translation offset.
         * @return Number of vertices accepted by the edit.
         */
        std::size_t translate_vertices(const std::vector<VertexHandle>& vertices, const glm::vec3& offset);

        /**
         * @brief Applies a matrix transform to multiple vertices.
         *
         * @param vertices Vertices to transform.
         * @param transform Transform matrix applied to each position.
         * @return Number of vertices accepted by the edit.
         */
        std::size_t transform_vertices(const std::vector<VertexHandle>& vertices, const glm::mat4& transform);

    private:
        LEM& mesh_;
        LEMDiff& diff_;
    };

}