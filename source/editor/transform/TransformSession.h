/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "editor/transform/TransformPivotResolver.h"
#include "editor/transform/TransformSpace.h"
#include "editor/transform/TransformTarget.h"

#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace locus::editor {

    class EditorScene;
    class SelectionState;

    /**
     * @brief Runtime state of an interactive transform session.
     */
    enum class TransformSessionState {
        /**
         * @brief No active transform session.
         */
        Idle,

        /**
         * @brief A transform session is currently previewing changes.
         */
        Active,

        /**
         * @brief The last session was confirmed.
         */
        Confirmed,

        /**
         * @brief The last session was cancelled.
         */
        Cancelled
    };

    /**
     * @brief Configuration used when starting a transform session.
     */
    struct TransformSessionOptions {
        /**
         * @brief Coordinate space used by the session.
         */
        TransformSpace space = TransformSpace::World;

        /**
         * @brief Pivot mode used by rotation and scale operations.
         */
        TransformPivotMode pivotMode = TransformPivotMode::SelectionCenter;

        /**
         * @brief Custom pivot used when pivotMode is Custom.
         */
        glm::vec3 customPivot{ 0.0f, 0.0f, 0.0f };
    };

    /**
     * @brief Interactive transform session used by tools and gizmos.
     *
     * The session captures initial node transforms, applies temporary preview
     * transforms directly to the editor scene, and can later confirm or restore the
     * captured state. It does not push commands to history by itself.
     */
    class TransformSession {
    public:
        /**
         * @brief Starts a session from the current object selection.
         *
         * @param scene Scene that owns the selected objects.
         * @param selection Selection state used to collect targets.
         * @param options Session configuration.
         * @return True when at least one valid target was captured.
         */
        bool begin(
            EditorScene& scene,
            const SelectionState& selection,
            const TransformSessionOptions& options = {});

        /**
         * @brief Starts a session from explicit target identifiers.
         *
         * @param scene Scene that owns the target objects.
         * @param targets Target node identifiers.
         * @param active Active object, or invalid when none is active.
         * @param options Session configuration.
         * @return True when at least one valid target was captured.
         */
        bool begin(
            EditorScene& scene,
            const std::vector<SceneNodeId>& targets,
            SceneNodeId active = {},
            const TransformSessionOptions& options = {});

        /**
         * @brief Clears the current session without touching the scene.
         */
        void clear();

        /**
         * @brief Checks whether the session is active.
         *
         * @return True when preview operations can be applied.
         */
        [[nodiscard]] bool is_active() const;

        /**
         * @brief Returns the current session state.
         *
         * @return Session state.
         */
        [[nodiscard]] TransformSessionState state() const;

        /**
         * @brief Returns the session transform space.
         *
         * @return Transform space.
         */
        [[nodiscard]] TransformSpace space() const;

        /**
         * @brief Returns the session pivot mode.
         *
         * @return Pivot mode.
         */
        [[nodiscard]] TransformPivotMode pivot_mode() const;

        /**
         * @brief Returns the shared pivot resolved when the session started.
         *
         * @return World-space pivot.
         */
        [[nodiscard]] const glm::vec3& pivot() const;

        /**
         * @brief Returns captured targets.
         *
         * @return Target list.
         */
        [[nodiscard]] const std::vector<TransformTarget>& targets() const;

        /**
         * @brief Applies an incremental translation preview.
         *
         * @param scene Scene that owns the target objects.
         * @param delta Translation delta.
         * @return True when at least one target was updated.
         */
        bool translate(EditorScene& scene, const glm::vec3& delta);

        /**
         * @brief Applies an incremental rotation preview.
         *
         * @param scene Scene that owns the target objects.
         * @param rotation Rotation delta.
         * @return True when at least one target was updated.
         */
        bool rotate(EditorScene& scene, const glm::quat& rotation);

        /**
         * @brief Applies an incremental scale preview.
         *
         * @param scene Scene that owns the target objects.
         * @param scale Scale delta.
         * @return True when at least one target was updated.
         */
        bool scale(EditorScene& scene, const glm::vec3& scale);

        /**
         * @brief Restores all captured initial transforms and cancels the session.
         *
         * @param scene Scene that owns the target objects.
         * @return True when at least one target was restored.
         */
        bool cancel(EditorScene& scene);

        /**
         * @brief Confirms the current preview and ends the session.
         *
         * The current scene transforms are kept as-is. The caller can inspect the
         * captured targets and dispatch undoable commands if desired.
         *
         * @return True when an active session was confirmed.
         */
        bool confirm();

        /**
         * @brief Checks whether any target differs from its initial transform.
         *
         * @return True when the session produced a transform change.
         */
        [[nodiscard]] bool has_changes() const;

    private:
        bool apply_world_matrix_to_target(
            EditorScene& scene,
            TransformTarget& target,
            const glm::mat4& worldMatrix);

        std::vector<TransformTarget> targets_{};
        TransformSessionState state_ = TransformSessionState::Idle;
        TransformSpace space_ = TransformSpace::World;
        TransformPivotMode pivotMode_ = TransformPivotMode::SelectionCenter;
        SceneNodeId active_{};
        glm::vec3 pivot_{ 0.0f, 0.0f, 0.0f };
    };

} // namespace locus::editor
