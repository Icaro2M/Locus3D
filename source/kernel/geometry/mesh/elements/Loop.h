/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"

namespace locus::kernel::geometry
{
    /**
     * @brief Stores a directed corner of a face boundary.
     *
     * A loop connects one vertex, one edge, and one face. Boundary links walk
     * around the face, while radial links walk across loops sharing the edge.
     */
    struct Loop
    {
        /**
         * @brief Vertex used by this directed corner.
         */
        VertexHandle vertex{};

        /**
         * @brief Edge used by this directed corner.
         */
        EdgeHandle edge{};

        /**
         * @brief Face that owns this directed corner.
         */
        FaceHandle face{};

        /**
         * @brief Next loop around the owning face boundary.
         */
        LoopHandle next{};

        /**
         * @brief Previous loop around the owning face boundary.
         */
        LoopHandle previous{};

        /**
         * @brief Next loop around the same edge.
         */
        LoopHandle radialNext{};

        /**
         * @brief Previous loop around the same edge.
         */
        LoopHandle radialPrevious{};

        /**
         * @brief True when the loop slot is no longer part of the active mesh.
         */
        bool deleted = false;
    };
}
