/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/transform/TransformTarget.h"

#include "editor/EditorTypes.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <cmath>

#include <glm/geometric.hpp>

namespace locus::editor {

    namespace {

        [[nodiscard]] bool vec3_nearly_equal(
            const glm::vec3& lhs,
            const glm::vec3& rhs,
            float epsilon)
        {
            return glm::length(lhs - rhs) <= epsilon;
        }

        [[nodiscard]] bool quat_nearly_equal(
            const glm::quat& lhs,
            const glm::quat& rhs,
            float epsilon)
        {
            const float dot = std::abs(glm::dot(lhs, rhs));
            return (1.0f - dot) <= epsilon;
        }

        void apply_transform_to_node(SceneNode& node, const NodeTransform& transform)
        {
            node.transform() = transform;
            node.mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);
        }

    } // namespace

    TransformTarget TransformTarget::capture(const SceneNode& node)
    {
        TransformTarget target{};
        target.node_ = node.id();
        target.initialTransform_ = NodeTransformSnapshot::capture(node.transform());
        target.previewTransform_ = target.initialTransform_;
        return target;
    }

    SceneNodeId TransformTarget::node() const
    {
        return node_;
    }

    const NodeTransformSnapshot& TransformTarget::initial_transform() const
    {
        return initialTransform_;
    }

    const NodeTransformSnapshot& TransformTarget::preview_transform() const
    {
        return previewTransform_;
    }

    void TransformTarget::set_preview_transform(const NodeTransform& transform)
    {
        previewTransform_ = NodeTransformSnapshot::capture(transform);
    }

    void TransformTarget::reset_preview()
    {
        previewTransform_ = initialTransform_;
    }

    bool TransformTarget::apply_preview(EditorScene& scene) const
    {
        SceneNode* sceneNode = scene.find_node(node_);
        if (!sceneNode) {
            return false;
        }

        apply_transform_to_node(*sceneNode, previewTransform_.transform);
        return true;
    }

    bool TransformTarget::restore(EditorScene& scene) const
    {
        SceneNode* sceneNode = scene.find_node(node_);
        if (!sceneNode) {
            return false;
        }

        apply_transform_to_node(*sceneNode, initialTransform_.transform);
        return true;
    }

    bool TransformTarget::has_transform_change(float epsilon) const
    {
        const NodeTransform& initial = initialTransform_.transform;
        const NodeTransform& preview = previewTransform_.transform;

        if (!vec3_nearly_equal(initial.position(), preview.position(), epsilon)) {
            return true;
        }

        if (!quat_nearly_equal(initial.rotation(), preview.rotation(), epsilon)) {
            return true;
        }

        if (!vec3_nearly_equal(initial.scale(), preview.scale(), epsilon)) {
            return true;
        }

        return false;
    }

} // namespace locus::editor