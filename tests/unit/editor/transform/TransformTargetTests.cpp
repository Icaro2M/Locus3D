/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TransformTestSuite.h"

#include "editor/EditorTypes.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"
#include "editor/transform/TransformTarget.h"

#include <cmath>

namespace {

constexpr float epsilon = 0.0001f;

[[nodiscard]] bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool near_vec3(const glm::vec3& lhs, const glm::vec3& rhs)
{
    return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

[[nodiscard]] bool near_quat(const glm::quat& lhs, const glm::quat& rhs)
{
    return near(lhs.w, rhs.w) &&
        near(lhs.x, rhs.x) &&
        near(lhs.y, rhs.y) &&
        near(lhs.z, rhs.z);
}

[[nodiscard]] locus::editor::NodeTransform make_transform(
    const glm::vec3& position,
    const glm::quat& rotation,
    const glm::vec3& scale)
{
    locus::editor::NodeTransform transform;
    transform.set_position(position);
    transform.set_rotation(rotation);
    transform.set_scale(scale);
    return transform;
}

[[nodiscard]] bool has_scene_render_picking(locus::editor::EditorDirtyFlags flags)
{
    return locus::editor::has_flag(flags, locus::editor::EditorDirtyFlags::Scene) &&
        locus::editor::has_flag(flags, locus::editor::EditorDirtyFlags::Render) &&
        locus::editor::has_flag(flags, locus::editor::EditorDirtyFlags::Picking);
}

} // namespace

namespace locus::tests {

TestResult run_transform_target_tests()
{
    editor::EditorScene scene;
    const editor::SceneNodeId nodeId = scene.create_empty("Transform Target");
    editor::SceneNode* node = scene.find_node(nodeId);
    if (!node) {
        return TestResult::fail("test scene should contain a transform target");
    }

    const editor::NodeTransform initial = make_transform(
        glm::vec3{ 1.0f, 2.0f, 3.0f },
        glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f },
        glm::vec3{ 2.0f, 3.0f, 4.0f });
    node->transform() = initial;

    editor::TransformTarget target = editor::TransformTarget::capture(*node);
    if (target.node() != nodeId ||
        target.has_transform_change() ||
        !near_vec3(target.initial_transform().transform.position(), initial.position()) ||
        !near_vec3(target.preview_transform().transform.scale(), initial.scale())) {
        return TestResult::fail("TransformTarget should capture the node id and initial transform");
    }

    const editor::NodeTransform preview = make_transform(
        glm::vec3{ -4.0f, 5.0f, 6.0f },
        glm::quat{ 0.0f, 0.0f, 1.0f, 0.0f },
        glm::vec3{ 0.5f, 2.0f, 3.0f });
    target.set_preview_transform(preview);
    if (!target.has_transform_change()) {
        return TestResult::fail("TransformTarget should report preview changes");
    }

    node->clear_dirty();
    if (!target.apply_preview(scene) ||
        !near_vec3(node->transform().position(), preview.position()) ||
        !near_quat(node->transform().rotation(), preview.rotation()) ||
        !near_vec3(node->transform().scale(), preview.scale()) ||
        !has_scene_render_picking(node->dirty_flags())) {
        return TestResult::fail("apply_preview should write the preview transform and mark the node dirty");
    }

    node->clear_dirty();
    if (!target.restore(scene) ||
        !near_vec3(node->transform().position(), initial.position()) ||
        !near_quat(node->transform().rotation(), initial.rotation()) ||
        !near_vec3(node->transform().scale(), initial.scale()) ||
        !has_scene_render_picking(node->dirty_flags())) {
        return TestResult::fail("restore should write the captured initial transform and mark the node dirty");
    }

    target.reset_preview();
    if (target.has_transform_change()) {
        return TestResult::fail("reset_preview should restore the preview snapshot to the initial transform");
    }

    scene.remove_node(nodeId);
    if (target.apply_preview(scene) || target.restore(scene)) {
        return TestResult::fail("TransformTarget should reject missing scene nodes");
    }

    return TestResult::pass();
}

} // namespace locus::tests
