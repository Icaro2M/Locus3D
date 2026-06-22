/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <optional>

namespace locus::kernel::geometry {

    class LEM;

    namespace editing::topology {

        /**
         * @brief Splits an edge at its midpoint.
         *
         * @param mesh Mesh that owns the edge.
         * @param diff Diff recorder that receives topology events.
         * @param edgeHandle Edge to split.
         * @return Created midpoint vertex, or an empty optional on failure.
         */
        std::optional<VertexHandle> split_edge(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle);

        /**
         * @brief Splits an edge at a parametric point.
         *
         * @param mesh Mesh that owns the edge.
         * @param diff Diff recorder that receives topology events.
         * @param edgeHandle Edge to split.
         * @param t Parametric position from vertexA to vertexB, clamped to [0, 1].
         * @return Created vertex, or an empty optional on failure.
         */
        std::optional<VertexHandle> split_edge_at_param(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle, float t);

        /**
         * @brief Splits a face by connecting two non-adjacent vertices in its boundary.
         *
         * @param mesh Mesh that owns the face.
         * @param diff Diff recorder that receives topology events.
         * @param faceHandle Face to split.
         * @param vertexA First boundary vertex.
         * @param vertexB Second boundary vertex.
         * @return Created diagonal edge, or an empty optional on failure.
         */
        std::optional<EdgeHandle> split_face(
            LEM& mesh,
            LEMDiff& diff,
            FaceHandle faceHandle,
            VertexHandle vertexA,
            VertexHandle vertexB);

    }

}