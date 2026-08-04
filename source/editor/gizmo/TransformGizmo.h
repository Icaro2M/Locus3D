/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/gizmo/GizmoConstraint.h"
#include "editor/gizmo/GizmoHit.h"
#include "editor/gizmo/GizmoMode.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::editor {

    /**
     * @brief Visual and hit-test configuration for transform gizmos.
     */
    struct TransformGizmoConfig {
        /**
         * @brief Length of axis handles in world units before visual scaling.
         */
        float axisLength = 1.35f;

        /**
         * @brief Ray distance accepted for axis handles.
         */
        float axisThickness = 0.06f;

        /**
         * @brief Visual shaft radius used to keep hit-test close to the rendered handle.
         */
        float shaftRadius = 0.025f;

        /**
         * @brief Length of translate arrow cones.
         */
        float arrowLength = 0.24f;

        /**
         * @brief Radius of translate arrow cone bases.
         */
        float arrowRadius = 0.085f;

        /**
         * @brief Size of plane handles in world units before visual scaling.
         */
        float planeSize = 0.32f;

        /**
         * @brief Offset of plane handles from the pivot.
         */
        float planeOffset = 0.22f;

        /**
         * @brief Additional accepted margin around plane handles.
         */
        float planePickingPadding = 0.025f;

        /**
         * @brief Radius of the center free-move/free-scale handle.
         */
        float centerRadius = 0.12f;

        /**
         * @brief Radius of rotation rings.
         */
        float rotationRadius = 1.05f;

        /**
         * @brief Ray distance accepted for rotation rings.
         */
        float rotationThickness = 0.06f;

        /**
         * @brief Visual tube radius of rotation rings.
         */
        float rotationTubeRadius = 0.018f;

        /**
         * @brief Extra scale applied to the view-facing rotation ring.
         */
        float viewRingScale = 1.12f;

        /**
         * @brief Radius used for scale endpoint handles in universal mode.
         */
        float scaleHandleRadius = 0.10f;

        /**
         * @brief Rendered size of scale endpoint cubes.
         */
        float scaleCubeSize = 0.18f;

        /**
         * @brief Extra picking margin applied to solid gizmo handles.
         */
        float pickingPadding = 0.025f;
    };

    /**
     * @brief Input used to hit-test a transform gizmo.
     */
    struct TransformGizmoHitTestInput {
        /**
         * @brief Active gizmo mode.
         */
        GizmoMode mode = GizmoMode::Translate;

        /**
         * @brief World-space gizmo pivot.
         */
        glm::vec3 pivot{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief World-space gizmo orientation.
         */
        glm::quat orientation{ 1.0f, 0.0f, 0.0f, 0.0f };

        /**
         * @brief Pointer ray in world coordinates.
         */
        GizmoRay ray{};

        /**
         * @brief Camera view direction in world coordinates.
         */
        glm::vec3 viewDirection{ 0.0f, 0.0f, -1.0f };

        /**
         * @brief Visual scale applied to the gizmo.
         */
        float visualScale = 1.0f;
    };

    /**
     * @brief Logical transform gizmo hit-tester.
     *
     * This class does not render and does not mutate scene nodes. It only maps a
     * pointer ray to a logical gizmo handle.
     */
    class TransformGizmo {
    public:
        /**
         * @brief Creates a gizmo with default hit-test configuration.
         */
        TransformGizmo() = default;

        /**
         * @brief Creates a gizmo with explicit hit-test configuration.
         *
         * @param config Gizmo configuration.
         */
        explicit TransformGizmo(const TransformGizmoConfig& config);

        /**
         * @brief Returns the active configuration.
         *
         * @return Gizmo configuration.
         */
        [[nodiscard]] const TransformGizmoConfig& config() const;

        /**
         * @brief Replaces the active configuration.
         *
         * @param config New configuration.
         */
        void set_config(const TransformGizmoConfig& config);

        /**
         * @brief Hit-tests the active gizmo mode.
         *
         * @param input Hit-test input.
         * @return Best logical hit, or invalid hit when nothing was hit.
         */
        [[nodiscard]] GizmoHit hit_test(const TransformGizmoHitTestInput& input) const;

    private:
        [[nodiscard]] GizmoHit hit_test_translate(
            const TransformGizmoHitTestInput& input) const;

        [[nodiscard]] GizmoHit hit_test_rotate(
            const TransformGizmoHitTestInput& input) const;

        [[nodiscard]] GizmoHit hit_test_scale(
            const TransformGizmoHitTestInput& input) const;

        [[nodiscard]] GizmoHit hit_test_universal(
            const TransformGizmoHitTestInput& input) const;

        TransformGizmoConfig config_{};
    };

} // namespace locus::editor
