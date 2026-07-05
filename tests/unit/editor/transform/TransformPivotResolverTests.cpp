/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TransformTestSuite.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"
#include "editor/transform/TransformPivotResolver.h"

#include <cmath>
#include <vector>

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

} // namespace

namespace locus::tests {

TestResult run_transform_pivot_resolver_tests()
{
    editor::EditorScene scene;
    const editor::SceneNodeId parentId = scene.create_empty("Parent");
    const editor::SceneNodeId childId = scene.create_empty("Child");
    const editor::SceneNodeId siblingId = scene.create_empty("Sibling");

    editor::SceneNode* parent = scene.find_node(parentId);
    editor::SceneNode* child = scene.find_node(childId);
    editor::SceneNode* sibling = scene.find_node(siblingId);
    if (!parent || !child || !sibling) {
        return TestResult::fail("test scene should contain pivot resolver nodes");
    }

    parent->transform().set_position(glm::vec3{ 10.0f, 0.0f, 0.0f });
    child->transform().set_position(glm::vec3{ 0.0f, 2.0f, 0.0f });
    sibling->transform().set_position(glm::vec3{ -2.0f, 4.0f, 0.0f });
    if (!scene.reparent(childId, parentId)) {
        return TestResult::fail("test scene should support parented nodes");
    }

    if (!near_vec3(
            editor::TransformPivotResolver::node_origin_position(scene, childId),
            glm::vec3{ 10.0f, 2.0f, 0.0f })) {
        return TestResult::fail("node_origin_position should include parent transforms");
    }

    child->pivot().custom = true;
    child->pivot().offset = glm::vec3{ 1.0f, 3.0f, 0.0f };
    if (!near_vec3(
            editor::TransformPivotResolver::node_pivot_position(scene, childId),
            glm::vec3{ 11.0f, 5.0f, 0.0f })) {
        return TestResult::fail("node_pivot_position should transform custom local pivot offsets");
    }

    const std::vector<editor::SceneNodeId> targets{
        childId,
        editor::SceneNodeId{ 999 },
        siblingId,
    };

    if (!near_vec3(
            editor::TransformPivotResolver::resolve(
                scene,
                targets,
                {},
                editor::TransformPivotMode::SelectionCenter),
            glm::vec3{ 4.5f, 4.5f, 0.0f })) {
        return TestResult::fail("SelectionCenter should average existing target pivots");
    }

    if (!near_vec3(
            editor::TransformPivotResolver::resolve(
                scene,
                targets,
                siblingId,
                editor::TransformPivotMode::ActiveObject),
            glm::vec3{ -2.0f, 4.0f, 0.0f })) {
        return TestResult::fail("ActiveObject should prefer the active node pivot");
    }

    if (!near_vec3(
            editor::TransformPivotResolver::resolve(
                scene,
                targets,
                editor::SceneNodeId{ 999 },
                editor::TransformPivotMode::ActiveObject),
            glm::vec3{ 4.5f, 4.5f, 0.0f })) {
        return TestResult::fail("ActiveObject should fall back to the selection center when missing");
    }

    if (!near_vec3(
            editor::TransformPivotResolver::resolve(
                scene,
                targets,
                siblingId,
                editor::TransformPivotMode::WorldOrigin),
            glm::vec3{ 0.0f, 0.0f, 0.0f })) {
        return TestResult::fail("WorldOrigin should resolve to the world origin");
    }

    if (!near_vec3(
            editor::TransformPivotResolver::resolve(
                scene,
                targets,
                siblingId,
                editor::TransformPivotMode::Custom,
                glm::vec3{ 7.0f, 8.0f, 9.0f }),
            glm::vec3{ 7.0f, 8.0f, 9.0f })) {
        return TestResult::fail("Custom should return the caller-provided pivot");
    }

    if (!near_vec3(
            editor::TransformPivotResolver::resolve(
                scene,
                {},
                {},
                editor::TransformPivotMode::SelectionCenter),
            glm::vec3{ 0.0f, 0.0f, 0.0f }) ||
        !near_vec3(
            editor::TransformPivotResolver::node_pivot_position(scene, editor::SceneNodeId{ 999 }),
            glm::vec3{ 0.0f, 0.0f, 0.0f })) {
        return TestResult::fail("pivot resolution should use world origin for empty or missing nodes");
    }

    return TestResult::pass();
}

} // namespace locus::tests
