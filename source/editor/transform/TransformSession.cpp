/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/transform/TransformSession.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"
#include "editor/selection/SelectionState.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::editor {

    namespace {

        [[nodiscard]] glm::vec3 rotate_point_around_pivot(
            const glm::vec3& point,
            const glm::vec3& pivot,
            const glm::quat& rotation)
        {
            return pivot + glm::normalize(rotation) * (point - pivot);
        }

        [[nodiscard]] glm::vec3 scale_point_around_pivot(
            const glm::vec3& point,
            const glm::vec3& pivot,
            const glm::vec3& scale)
        {
            return pivot + ((point - pivot) * scale);
        }

    } // namespace

    bool TransformSession::begin(
        EditorScene& scene,
        const SelectionState& selection,
        const TransformSessionOptions& options)
    {
        return begin(
            scene,
            selection.objects().selected(),
            selection.objects().active(),
            options);
    }

    bool TransformSession::begin(
        EditorScene& scene,
        const std::vector<SceneNodeId>& targets,
        SceneNodeId active,
        const TransformSessionOptions& options)
    {
        clear();

        space_ = options.space;
        pivotMode_ = options.pivotMode;
        active_ = active;

        std::vector<SceneNodeId> capturedIds{};
        capturedIds.reserve(targets.size());

        targets_.reserve(targets.size());

        for (SceneNodeId id : targets) {
            SceneNode* node = scene.find_node(id);
            if (!node || !node->is_selectable()) {
                continue;
            }

            targets_.push_back(TransformTarget::capture(*node));
            capturedIds.push_back(id);
        }

        if (targets_.empty()) {
            state_ = TransformSessionState::Idle;
            return false;
        }

        pivot_ = TransformPivotResolver::resolve(
            scene,
            capturedIds,
            active_,
            pivotMode_,
            options.customPivot);

        state_ = TransformSessionState::Active;
        return true;
    }

    void TransformSession::clear()
    {
        targets_.clear();
        state_ = TransformSessionState::Idle;
        space_ = TransformSpace::World;
        pivotMode_ = TransformPivotMode::SelectionCenter;
        active_ = {};
        pivot_ = glm::vec3{ 0.0f, 0.0f, 0.0f };
    }

    bool TransformSession::is_active() const
    {
        return state_ == TransformSessionState::Active;
    }

    TransformSessionState TransformSession::state() const
    {
        return state_;
    }

    TransformSpace TransformSession::space() const
    {
        return space_;
    }

    TransformPivotMode TransformSession::pivot_mode() const
    {
        return pivotMode_;
    }

    const glm::vec3& TransformSession::pivot() const
    {
        return pivot_;
    }

    const std::vector<TransformTarget>& TransformSession::targets() const
    {
        return targets_;
    }

    bool TransformSession::translate(EditorScene& scene, const glm::vec3& delta)
    {
        if (!is_active()) {
            return false;
        }

        bool updated = false;

        for (TransformTarget& target : targets_) {
            SceneNode* node = scene.find_node(target.node());
            if (!node) {
                continue;
            }

            NodeTransform transform = node->transform();

            if (space_ == TransformSpace::Local) {
                transform.translate(transform.rotation() * delta);
            }
            else {
                transform.translate(delta);
            }

            target.set_preview_transform(transform);
            updated = target.apply_preview(scene) || updated;
        }

        return updated;
    }

    bool TransformSession::rotate(EditorScene& scene, const glm::quat& rotation)
    {
        if (!is_active()) {
            return false;
        }

        const glm::quat normalizedRotation = glm::normalize(rotation);

        bool updated = false;

        for (TransformTarget& target : targets_) {
            SceneNode* node = scene.find_node(target.node());
            if (!node) {
                continue;
            }

            NodeTransform transform = node->transform();

            if (space_ == TransformSpace::Local) {
                transform.set_rotation(glm::normalize(transform.rotation() * normalizedRotation));
            }
            else {
                const glm::vec3 pivot =
                    pivotMode_ == TransformPivotMode::IndividualOrigins
                    ? TransformPivotResolver::node_pivot_position(scene, target.node())
                    : pivot_;

                transform.set_position(rotate_point_around_pivot(
                    transform.position(),
                    pivot,
                    normalizedRotation));

                transform.set_rotation(glm::normalize(normalizedRotation * transform.rotation()));
            }

            target.set_preview_transform(transform);
            updated = target.apply_preview(scene) || updated;
        }

        return updated;
    }

    bool TransformSession::scale(EditorScene& scene, const glm::vec3& scale)
    {
        if (!is_active()) {
            return false;
        }

        bool updated = false;

        for (TransformTarget& target : targets_) {
            SceneNode* node = scene.find_node(target.node());
            if (!node) {
                continue;
            }

            NodeTransform transform = node->transform();

            if (space_ == TransformSpace::Local) {
                transform.set_scale(transform.scale() * scale);
            }
            else {
                const glm::vec3 pivot =
                    pivotMode_ == TransformPivotMode::IndividualOrigins
                    ? TransformPivotResolver::node_pivot_position(scene, target.node())
                    : pivot_;

                transform.set_position(scale_point_around_pivot(
                    transform.position(),
                    pivot,
                    scale));

                transform.set_scale(transform.scale() * scale);
            }

            target.set_preview_transform(transform);
            updated = target.apply_preview(scene) || updated;
        }

        return updated;
    }

    bool TransformSession::cancel(EditorScene& scene)
    {
        if (!is_active()) {
            return false;
        }

        bool restored = false;

        for (const TransformTarget& target : targets_) {
            restored = target.restore(scene) || restored;
        }

        state_ = TransformSessionState::Cancelled;
        return restored;
    }

    bool TransformSession::confirm()
    {
        if (!is_active()) {
            return false;
        }

        state_ = TransformSessionState::Confirmed;
        return true;
    }

    bool TransformSession::has_changes() const
    {
        for (const TransformTarget& target : targets_) {
            if (target.has_transform_change()) {
                return true;
            }
        }

        return false;
    }

} // namespace locus::editor