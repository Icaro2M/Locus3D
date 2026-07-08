/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/gizmo/GizmoAxis.h"
#include "editor/gizmo/GizmoMode.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::editor {

    /**
     * @brief Ray used by gizmo math.
     */
    struct GizmoRay {
        /**
         * @brief Ray origin in world coordinates.
         */
        glm::vec3 origin{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Ray direction in world coordinates.
         */
        glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
    };

    /**
     * @brief Input used to solve one constrained gizmo drag step.
     */
    struct GizmoConstraintInput {
        /**
         * @brief Current gizmo mode.
         */
        GizmoMode mode = GizmoMode::Translate;

        /**
         * @brief Active handle.
         */
        GizmoAxis axis = GizmoAxis::None;

        /**
         * @brief World-space transform pivot.
         */
        glm::vec3 pivot{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief World-space gizmo orientation.
         */
        glm::quat orientation{ 1.0f, 0.0f, 0.0f, 0.0f };

        /**
         * @brief World-space point captured when dragging started.
         */
        glm::vec3 startPoint{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Pointer ray captured when dragging started.
         */
        GizmoRay startRay{};

        /**
         * @brief Current pointer ray.
         */
        GizmoRay currentRay{};

        /**
         * @brief Camera forward/view direction in world coordinates.
         */
        glm::vec3 viewDirection{ 0.0f, 0.0f, -1.0f };

        /**
         * @brief Camera right direction in world coordinates.
         */
        glm::vec3 viewRight{ 1.0f, 0.0f, 0.0f };

        /**
         * @brief Camera up direction in world coordinates.
         */
        glm::vec3 viewUp{ 0.0f, 1.0f, 0.0f };

        /**
         * @brief Multiplier used by screen-space scale handles.
         */
        float scaleSensitivity = 1.0f;

        /**
         * @brief Multiplier used by screen-space rotation handles.
         */
        float rotationSensitivity = 1.0f;
    };

    /**
     * @brief Output produced by a constrained gizmo drag step.
     */
    struct GizmoConstraintResult {
        /**
         * @brief Whether the result is usable.
         */
        bool valid = false;

        /**
         * @brief Translation delta for translate mode.
         */
        glm::vec3 translation{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Scale factor for scale mode.
         */
        glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

        /**
         * @brief Rotation delta for rotate mode.
         */
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };

        /**
         * @brief Signed scalar amount along the active constraint.
         */
        float signedAmount = 0.0f;

        /**
         * @brief Signed rotation angle in radians.
         */
        float angle = 0.0f;

        /**
         * @brief Current constrained world point.
         */
        glm::vec3 constrainedPoint{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Creates an invalid result.
         *
         * @return Invalid result.
         */
        [[nodiscard]] static GizmoConstraintResult none()
        {
            return {};
        }

        /**
         * @brief Checks whether the result is valid.
         *
         * @return True when valid.
         */
        [[nodiscard]] bool is_valid() const
        {
            return valid;
        }
    };

    /**
     * @brief Solves transform constraints for gizmo drags.
     */
    class GizmoConstraint {
    public:
        /**
         * @brief Returns the world direction represented by a single axis handle.
         *
         * @param axis Handle to inspect.
         * @return Unit axis vector, or zero for non-axis handles.
         */
        [[nodiscard]] static glm::vec3 axis_vector(GizmoAxis axis);

        /**
         * @brief Returns the oriented world direction represented by a single axis handle.
         *
         * @param axis Handle to inspect.
         * @param orientation Gizmo world orientation.
         * @return Unit axis vector, or zero for non-axis handles.
         */
        [[nodiscard]] static glm::vec3 axis_vector(
            GizmoAxis axis,
            const glm::quat& orientation);

        /**
         * @brief Returns the normal of a plane handle.
         *
         * @param axis Plane handle.
         * @param viewDirection Fallback normal for view/free handles.
         * @return Unit plane normal.
         */
        [[nodiscard]] static glm::vec3 plane_normal(
            GizmoAxis axis,
            const glm::vec3& viewDirection);

        /**
         * @brief Returns the oriented normal of a plane handle.
         *
         * @param axis Plane handle.
         * @param orientation Gizmo world orientation.
         * @param viewDirection Fallback normal for view/free handles.
         * @return Unit plane normal.
         */
        [[nodiscard]] static glm::vec3 plane_normal(
            GizmoAxis axis,
            const glm::quat& orientation,
            const glm::vec3& viewDirection);

        /**
         * @brief Solves a translation drag.
         *
         * @param input Constraint input.
         * @return Translation constraint result.
         */
        [[nodiscard]] static GizmoConstraintResult solve_translation(
            const GizmoConstraintInput& input);

        /**
         * @brief Solves a scale drag.
         *
         * @param input Constraint input.
         * @return Scale constraint result.
         */
        [[nodiscard]] static GizmoConstraintResult solve_scale(
            const GizmoConstraintInput& input);

        /**
         * @brief Solves a rotation drag.
         *
         * @param input Constraint input.
         * @return Rotation constraint result.
         */
        [[nodiscard]] static GizmoConstraintResult solve_rotation(
            const GizmoConstraintInput& input);
    };

} // namespace locus::editor