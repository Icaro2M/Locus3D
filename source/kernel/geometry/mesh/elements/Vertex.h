/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/glm.hpp>

namespace locus::kernel::geometry
{
    /**
     * @brief Stores geometric and topological data for a mesh vertex.
     */
    struct Vertex
    {
        /**
         * @brief Vertex position in object space.
         */
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief One incident edge connected to this vertex.
         *
         * @note Additional incident edges can be found by traversing mesh
         * topology from this edge.
         */
        EdgeHandle edge{};

        /**
         * @brief True when the vertex is selected by editing tools.
         */
        bool selected = false;

        /**
         * @brief True when the vertex should be ignored by visible editing views.
         */
        bool hidden = false;

        /**
         * @brief True when the vertex slot is no longer part of the active mesh.
         */
        bool deleted = false;
    };
}
