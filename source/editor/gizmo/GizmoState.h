/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/gizmo/GizmoHit.h"
#include "editor/gizmo/GizmoMode.h"
#include "editor/transform/TransformSpace.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::editor {

    /**
     * @brief Runtime state shared by gizmo controllers, tools, and render adapters.
     */
    struct GizmoState {
        /**
         * @brief Whether the gizmo can be shown and interacted with.
         */
        bool enabled = true;

        /**
         * @brief Whether the gizmo should be drawn.
         */
        bool visible = true;

        /**
         * @brief Whether the user is currently dragging a gizmo handle.
         */
        bool dragging = false;

        /**
         * @brief Active gizmo mode.
         */
        GizmoMode mode = GizmoMode::Translate;

        /**
         * @brief Coordinate space used by the gizmo.
         */
        TransformSpace space = TransformSpace::World;

        /**
         * @brief World-space gizmo pivot.
         */
        glm::vec3 pivot{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Gizmo world orientation.
         */
        glm::quat orientation{ 1.0f, 0.0f, 0.0f, 0.0f };

        /**
         * @brief Visual size multiplier used by render adapters.
         */
        float visualScale = 1.0f;

        /**
         * @brief Currently hovered handle.
         */
        GizmoHit hovered{};

        /**
         * @brief Currently active handle.
         */
        GizmoHit active{};

        /**
         * @brief Clears hover state.
         */
        void clear_hover()
        {
            hovered = GizmoHit::none();
        }

        /**
         * @brief Clears active dragging state.
         */
        void clear_active()
        {
            active = GizmoHit::none();
            dragging = false;
        }

        /**
         * @brief Checks whether the gizmo can be interacted with.
         *
         * @return True when enabled and visible.
         */
        [[nodiscard]] bool can_interact() const
        {
            return enabled && visible;
        }
    };

} // namespace locus::editor