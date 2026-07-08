/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/gizmo/GizmoConstraint.h"
#include "editor/gizmo/GizmoHit.h"
#include "editor/gizmo/GizmoMode.h"
#include "editor/gizmo/GizmoSnap.h"
#include "editor/gizmo/GizmoState.h"
#include "editor/gizmo/TransformGizmo.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/transform/TransformSession.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::editor {

    class EditorScene;
    class SelectionState;
    class SnapSettings;
    class SnapSolver;

    /**
     * @brief Normalized pointer/camera data consumed by the editor gizmo.
     *
     * This structure intentionally does not expose GLFW, ImGui, or platform input
     * events. The application layer is responsible for converting raw input into
     * rays and camera vectors.
     */
    struct GizmoPointerInput {
        /**
         * @brief Pointer ray in world coordinates.
         */
        GizmoRay ray{};

        /**
         * @brief Camera view direction in world coordinates.
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
         * @brief Visual scale applied to the gizmo.
         */
        float visualScale = 1.0f;
    };

    /**
     * @brief Input used to update hover state.
     */
    struct GizmoHoverInput {
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
         * @brief Pointer/camera input.
         */
        GizmoPointerInput pointer{};
    };

    /**
     * @brief Input used to start a gizmo drag.
     */
    struct GizmoBeginDragInput {
        /**
         * @brief Scene that owns transformed nodes.
         */
        EditorScene* scene = nullptr;

        /**
         * @brief Selection used to collect transform targets.
         */
        const SelectionState* selection = nullptr;

        /**
         * @brief Active gizmo mode.
         */
        GizmoMode mode = GizmoMode::Translate;

        /**
         * @brief World-space gizmo orientation.
         */
        glm::quat orientation{ 1.0f, 0.0f, 0.0f, 0.0f };

        /**
         * @brief Transform session options.
         */
        TransformSessionOptions sessionOptions{};

        /**
         * @brief Pointer/camera input.
         */
        GizmoPointerInput pointer{};

        /**
         * @brief Optional snap settings.
         */
        const SnapSettings* snapSettings = nullptr;

        /**
         * @brief Optional snap solver.
         */
        const SnapSolver* snapSolver = nullptr;
    };

    /**
     * @brief Input used to start a gizmo drag from explicit targets.
     */
    struct GizmoBeginDragTargetsInput {
        /**
         * @brief Scene that owns transformed nodes.
         */
        EditorScene* scene = nullptr;

        /**
         * @brief Explicit target nodes.
         */
        std::vector<SceneNodeId> targets{};

        /**
         * @brief Active node, or invalid when none is active.
         */
        SceneNodeId active{};

        /**
         * @brief Active gizmo mode.
         */
        GizmoMode mode = GizmoMode::Translate;

        /**
         * @brief World-space gizmo orientation.
         */
        glm::quat orientation{ 1.0f, 0.0f, 0.0f, 0.0f };

        /**
         * @brief Transform session options.
         */
        TransformSessionOptions sessionOptions{};

        /**
         * @brief Pointer/camera input.
         */
        GizmoPointerInput pointer{};

        /**
         * @brief Optional snap settings.
         */
        const SnapSettings* snapSettings = nullptr;

        /**
         * @brief Optional snap solver.
         */
        const SnapSolver* snapSolver = nullptr;
    };

    /**
     * @brief Input used while dragging an active gizmo handle.
     */
    struct GizmoDragInput {
        /**
         * @brief Pointer/camera input.
         */
        GizmoPointerInput pointer{};

        /**
         * @brief Optional snap settings override.
         */
        const SnapSettings* snapSettings = nullptr;

        /**
         * @brief Optional snap solver override.
         */
        const SnapSolver* snapSolver = nullptr;
    };

    /**
     * @brief Result of one controller interaction step.
     */
    struct GizmoControllerResult {
        /**
         * @brief Whether the operation succeeded.
         */
        bool success = false;

        /**
         * @brief Whether the scene transform preview changed.
         */
        bool changed = false;

        /**
         * @brief Hit used by the operation.
         */
        GizmoHit hit{};

        /**
         * @brief Last constraint result before incremental conversion.
         */
        GizmoConstraintResult constraint{};

        /**
         * @brief Optional message useful for smoke tests and diagnostics.
         */
        const char* message = "";
    };

    /**
     * @brief Coordinates transform gizmo interaction and transform preview sessions.
     */
    class GizmoController {
    public:
        /**
         * @brief Creates a controller with default transform gizmo settings.
         */
        GizmoController() = default;

        /**
         * @brief Creates a controller with an explicit transform gizmo.
         *
         * @param gizmo Transform gizmo hit-tester.
         */
        explicit GizmoController(const TransformGizmo& gizmo);

        /**
         * @brief Returns read-only gizmo state.
         *
         * @return Current state.
         */
        [[nodiscard]] const GizmoState& state() const;

        /**
         * @brief Returns mutable gizmo state.
         *
         * @return Current state.
         */
        [[nodiscard]] GizmoState& state();

        /**
         * @brief Returns read-only access to the transform session.
         *
         * @return Transform session.
         */
        [[nodiscard]] const TransformSession& session() const;

        /**
         * @brief Returns mutable access to the transform session.
         *
         * @return Transform session.
         */
        [[nodiscard]] TransformSession& session();

        /**
         * @brief Returns read-only access to the transform gizmo.
         *
         * @return Transform gizmo.
         */
        [[nodiscard]] const TransformGizmo& gizmo() const;

        /**
         * @brief Returns mutable access to the transform gizmo.
         *
         * @return Transform gizmo.
         */
        [[nodiscard]] TransformGizmo& gizmo();

        /**
         * @brief Updates hovered handle from normalized pointer input.
         *
         * @param input Hover input.
         * @return Hovered hit, or invalid hit.
         */
        [[nodiscard]] GizmoHit update_hover(const GizmoHoverInput& input);

        /**
         * @brief Starts a drag from the current object selection.
         *
         * @param input Begin-drag input.
         * @return Operation result.
         */
        [[nodiscard]] GizmoControllerResult begin_drag(const GizmoBeginDragInput& input);

        /**
         * @brief Starts a drag from explicit target nodes.
         *
         * @param input Begin-drag input.
         * @return Operation result.
         */
        [[nodiscard]] GizmoControllerResult begin_drag(const GizmoBeginDragTargetsInput& input);

        /**
         * @brief Updates an active drag and applies an incremental transform preview.
         *
         * @param scene Scene that owns transformed nodes.
         * @param input Drag input.
         * @return Operation result.
         */
        [[nodiscard]] GizmoControllerResult update_drag(
            EditorScene& scene,
            const GizmoDragInput& input);

        /**
         * @brief Confirms the active transform session.
         *
         * @return True when an active session was confirmed.
         */
        bool end_drag();

        /**
         * @brief Cancels the active transform session and restores captured transforms.
         *
         * @param scene Scene that owns transformed nodes.
         * @return True when at least one target was restored.
         */
        bool cancel_drag(EditorScene& scene);

        /**
         * @brief Clears controller state without modifying the scene.
         */
        void clear();

    private:
        [[nodiscard]] GizmoControllerResult begin_drag_from_hit(
            EditorScene& scene,
            const GizmoHit& hit,
            const GizmoBeginDragInput& input);

        [[nodiscard]] GizmoControllerResult begin_drag_from_hit(
            EditorScene& scene,
            const GizmoHit& hit,
            const GizmoBeginDragTargetsInput& input);

        [[nodiscard]] GizmoConstraintInput make_constraint_input(
            const GizmoPointerInput& pointer) const;

        [[nodiscard]] GizmoControllerResult apply_constraint(
            EditorScene& scene,
            const GizmoConstraintResult& constraint,
            const SnapSettings* snapSettings,
            const SnapSolver* snapSolver);

        [[nodiscard]] GizmoConstraintResult solve_constraint(
            const GizmoPointerInput& pointer) const;

        void reset_incremental_state();

        TransformGizmo gizmo_{};
        GizmoState state_{};
        TransformSession session_{};

        GizmoRay startRay_{};
        glm::vec3 startPoint_{ 0.0f, 0.0f, 0.0f };
        glm::vec3 startPivot_{ 0.0f, 0.0f, 0.0f };
        glm::vec3 lastTranslation_{ 0.0f, 0.0f, 0.0f };
        glm::vec3 lastScale_{ 1.0f, 1.0f, 1.0f };
        glm::quat lastRotation_{ 1.0f, 0.0f, 0.0f, 0.0f };

        const SnapSettings* activeSnapSettings_ = nullptr;
        const SnapSolver* activeSnapSolver_ = nullptr;
    };

} // namespace locus::editor