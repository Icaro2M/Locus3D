/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <glm/vec3.hpp>

namespace locus::editor {

    /**
     * @brief Pivot information used by transform tools and gizmos.
     */
    struct NodePivot {
        /**
         * @brief Pivot offset in node-local space.
         */
        glm::vec3 offset{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief True when the node uses a custom pivot offset.
         */
        bool custom = false;

        /**
         * @brief Clears the custom pivot and returns to the default origin pivot.
         */
        void clear()
        {
            offset = glm::vec3{ 0.0f, 0.0f, 0.0f };
            custom = false;
        }
    };

}