/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/vec3.hpp>

#include <cstdint>

namespace locus::kernel::geometry {

    /**
     * @brief Stores topological, geometric, and attribute data for a mesh face.
     */
    struct Face {
        /**
         * @brief One boundary loop belonging to this face.
         *
         * @note The full face boundary can be reached through the loop's next
         * and previous links.
         */
        LoopHandle loop{};

        /**
         * @brief Face normal used by geometry tools and default shading.
         */
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };

        /**
         * @brief Internal user-defined tag used by editing and modeling tools.
         */
        std::uint32_t tag = 0;

        /**
         * @brief True when the face is selected by editing tools.
         */
        bool selected = false;

        /**
         * @brief True when the face should be ignored by visible editing views.
         */
        bool hidden = false;

        /**
         * @brief True when the face slot is no longer part of the active mesh.
         */
        bool deleted = false;
    };

}