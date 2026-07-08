/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/gizmo/GizmoConstraint.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/snapping/SnapContext.h"
#include "editor/snapping/SnapResult.h"
#include "editor/snapping/SnapSettings.h"
#include "editor/transform/TransformSpace.h"

#include <glm/glm.hpp>

namespace locus::editor {

    class EditorScene;
    class SnapSolver;

    /**
     * @brief Input used to snap one gizmo transform result.
     */
    struct GizmoSnapRequest {
        /**
         * @brief Optional scene used by snap providers.
         */
        const EditorScene* scene = nullptr;

        /**
         * @brief Optional snap settings.
         */
        const SnapSettings* settings = nullptr;

        /**
         * @brief Optional snap solver.
         */
        const SnapSolver* solver = nullptr;

        /**
         * @brief Original world position before the gizmo drag.
         */
        glm::vec3 originalPosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Current candidate world position before snapping.
         */
        glm::vec3 candidatePosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Reference origin used by increment/angle snapping.
         */
        glm::vec3 referenceOrigin{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief View direction used by screen-aware providers.
         */
        glm::vec3 viewDirection{ 0.0f, 0.0f, -1.0f };

        /**
         * @brief Active node affected by the operation.
         */
        SceneNodeId activeNode{};

        /**
         * @brief Transform space used by the interaction.
         */
        TransformSpace space = TransformSpace::World;

        /**
         * @brief Optional snap distance override.
         */
        float maxDistanceOverride = -1.0f;
    };

    /**
     * @brief Snapped gizmo transform result.
     */
    struct GizmoSnapResult {
        /**
         * @brief Whether snapping changed the candidate.
         */
        bool snapped = false;

        /**
         * @brief Snapped world position.
         */
        glm::vec3 snappedPosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Translation delta from originalPosition to snappedPosition.
         */
        glm::vec3 delta{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Raw snap provider result.
         */
        SnapResult source{};
    };

    /**
     * @brief Applies editor snapping to gizmo operations.
     */
    class GizmoSnap {
    public:
        /**
         * @brief Snaps a world-space position using the configured SnapSolver.
         *
         * @param request Snap request.
         * @return Snapped result.
         */
        [[nodiscard]] static GizmoSnapResult snap_position(
            const GizmoSnapRequest& request);

        /**
         * @brief Snaps a translation constraint result.
         *
         * @param request Base snap request.
         * @param constraint Constraint result to snap.
         * @return Snapped result.
         */
        [[nodiscard]] static GizmoSnapResult snap_translation(
            const GizmoSnapRequest& request,
            const GizmoConstraintResult& constraint);

        /**
         * @brief Snaps an angle value using SnapSettings::angle_increment.
         *
         * @param radians Angle in radians.
         * @param settings Snap settings.
         * @return Snapped angle in radians.
         */
        [[nodiscard]] static float snap_angle(
            float radians,
            const SnapSettings& settings);
    };

} // namespace locus::editor