/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/gizmo/GizmoAxis.h"
#include "editor/gizmo/GizmoMode.h"

#include <glm/glm.hpp>

namespace locus::editor {

    /**
     * @brief Result of a logical gizmo hit-test.
     */
    struct GizmoHit {
        /**
         * @brief Whether the hit is valid.
         */
        bool valid = false;

        /**
         * @brief Gizmo mode hit by the pointer.
         */
        GizmoMode mode = GizmoMode::None;

        /**
         * @brief Logical handle hit by the pointer.
         */
        GizmoAxis axis = GizmoAxis::None;

        /**
         * @brief World-space position associated with the hit.
         */
        glm::vec3 worldPosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Distance from the pointer ray to the handle.
         */
        float distance = 0.0f;

        /**
         * @brief Depth used to sort hits from camera to far plane.
         */
        float depth = 0.0f;

        /**
         * @brief Creates an invalid hit.
         *
         * @return Invalid hit.
         */
        [[nodiscard]] static GizmoHit none()
        {
            return {};
        }

        /**
         * @brief Creates a valid hit.
         *
         * @param mode Hit gizmo mode.
         * @param axis Hit gizmo handle.
         * @param worldPosition Hit position in world coordinates.
         * @param distance Ray-to-handle distance.
         * @param depth Camera depth.
         * @return Valid hit.
         */
        [[nodiscard]] static GizmoHit make(
            GizmoMode mode,
            GizmoAxis axis,
            const glm::vec3& worldPosition,
            float distance,
            float depth)
        {
            GizmoHit hit{};
            hit.valid = true;
            hit.mode = mode;
            hit.axis = axis;
            hit.worldPosition = worldPosition;
            hit.distance = distance;
            hit.depth = depth;
            return hit;
        }

        /**
         * @brief Checks whether the hit is valid.
         *
         * @return True when valid.
         */
        [[nodiscard]] bool is_valid() const
        {
            return valid;
        }
    };

} // namespace locus::editor