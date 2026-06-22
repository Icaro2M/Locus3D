/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"

#include <cstdint>

namespace locus::kernel::geometry {

    /**
     * @brief Stores topological and attribute data for a mesh edge.
     */
    struct Edge {
        /**
         * @brief First endpoint vertex of the edge.
         */
        VertexHandle vertexA{};

        /**
         * @brief Second endpoint vertex of the edge.
         */
        VertexHandle vertexB{};

        /**
         * @brief One loop that uses this edge.
         *
         * @note Other loops around the same edge can be reached through radial
         * loop links.
         */
        LoopHandle loop{};

        /**
         * @brief True when adjacent faces should be smoothed across this edge.
         */
        bool smooth = false;

        /**
         * @brief Crease strength used by modeling and subdivision tools.
         */
        float crease = 0.0f;

        /**
         * @brief Internal user-defined tag used by editing and modeling tools.
         */
        std::uint32_t tag = 0;

        /**
         * @brief True when the edge is selected by editing tools.
         */
        bool selected = false;

        /**
         * @brief True when the edge should be ignored by visible editing views.
         */
        bool hidden = false;

        /**
         * @brief True when the edge slot is no longer part of the active mesh.
         */
        bool deleted = false;
    };

}