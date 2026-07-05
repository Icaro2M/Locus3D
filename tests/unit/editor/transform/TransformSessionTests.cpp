/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TransformTestSuite.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"
#include "editor/selection/SelectionState.h"
#include "editor/transform/TransformSession.h"

#include <cmath>
#include <vector>

#include <glm/gtc/quaternion.hpp>

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

} // namespace

namespace locus::tests {

TestResult run_transform_session_tests()
{
    editor::EditorScene scene;
    const editor::SceneNodeId firstId = scene.create_empty("First");
    const editor::SceneNodeId secondId = scene.create_empty("Second");
    const editor::SceneNodeId lockedId = scene.create_empty("Locked");

    editor::SceneNode* first = scene.find_node(firstId);
    editor::SceneNode* second = scene.find_node(secondId);
    editor::SceneNode* locked = scene.find_node(lockedId);
    if (!first || !second || !locked) {
        return TestResult::fail("test scene should contain transform session nodes");
    }

    first->transform().set_position(glm::vec3{ 1.0f, 0.0f, 0.0f });
    second->transform().set_position(glm::vec3{ 3.0f, 0.0f, 0.0f });
    locked->transform().set_position(glm::vec3{ 9.0f, 0.0f, 0.0f });
    locked->metadata().locked = true;

    editor::TransformSession session;
    if (session.is_active() ||
        session.confirm() ||
        session.translate(scene, glm::vec3{ 1.0f }) ||
        session.cancel(scene)) {
        return TestResult::fail("idle TransformSession should reject preview, confirm, and cancel operations");
    }

    const std::vector<editor::SceneNodeId> ids{
        firstId,
        lockedId,
        editor::SceneNodeId{ 999 },
        secondId,
    };
    editor::TransformSessionOptions options;
    options.pivotMode = editor::TransformPivotMode::SelectionCenter;

    if (!session.begin(scene, ids, secondId, options) ||
        !session.is_active() ||
        session.state() != editor::TransformSessionState::Active ||
        session.targets().size() != 2 ||
        !near_vec3(session.pivot(), glm::vec3{ 2.0f, 0.0f, 0.0f }) ||
        session.has_changes()) {
        return TestResult::fail("begin should capture selectable targets and resolve the selection center pivot");
    }

    if (!session.translate(scene, glm::vec3{ 1.0f, 2.0f, 0.0f }) ||
        !near_vec3(first->transform().position(), glm::vec3{ 2.0f, 2.0f, 0.0f }) ||
        !near_vec3(second->transform().position(), glm::vec3{ 4.0f, 2.0f, 0.0f }) ||
        !near_vec3(locked->transform().position(), glm::vec3{ 9.0f, 0.0f, 0.0f }) ||
        !session.has_changes()) {
        return TestResult::fail("translate should update only captured selectable targets");
    }

    if (!session.cancel(scene) ||
        session.is_active() ||
        session.state() != editor::TransformSessionState::Cancelled ||
        !near_vec3(first->transform().position(), glm::vec3{ 1.0f, 0.0f, 0.0f }) ||
        !near_vec3(second->transform().position(), glm::vec3{ 3.0f, 0.0f, 0.0f })) {
        return TestResult::fail("cancel should restore captured transforms and end the active session");
    }

    if (session.translate(scene, glm::vec3{ 1.0f }) || session.confirm()) {
        return TestResult::fail("cancelled TransformSession should reject further operations");
    }

    editor::SelectionState selection;
    selection.objects().set({ firstId, secondId }, firstId);
    options.pivotMode = editor::TransformPivotMode::ActiveObject;
    if (!session.begin(scene, selection, options) ||
        session.pivot_mode() != editor::TransformPivotMode::ActiveObject ||
        !near_vec3(session.pivot(), glm::vec3{ 1.0f, 0.0f, 0.0f })) {
        return TestResult::fail("begin from SelectionState should use selected objects and active pivot");
    }

    const glm::quat halfTurn = glm::angleAxis(3.14159265358979323846f, glm::vec3{ 0.0f, 0.0f, 1.0f });
    if (!session.rotate(scene, halfTurn) ||
        !near_vec3(first->transform().position(), glm::vec3{ 1.0f, 0.0f, 0.0f }) ||
        !near_vec3(second->transform().position(), glm::vec3{ -1.0f, 0.0f, 0.0f })) {
        return TestResult::fail("world rotation should rotate positions around the resolved pivot");
    }

    if (!session.confirm() ||
        session.is_active() ||
        session.state() != editor::TransformSessionState::Confirmed ||
        !near_vec3(second->transform().position(), glm::vec3{ -1.0f, 0.0f, 0.0f })) {
        return TestResult::fail("confirm should keep preview transforms and end the active session");
    }

    first->transform().reset();
    first->transform().set_rotation(halfTurn);
    session.clear();
    options.space = editor::TransformSpace::Local;
    options.pivotMode = editor::TransformPivotMode::SelectionCenter;
    if (!session.begin(scene, std::vector<editor::SceneNodeId>{ firstId }, firstId, options) ||
        session.space() != editor::TransformSpace::Local ||
        !session.translate(scene, glm::vec3{ 1.0f, 0.0f, 0.0f }) ||
        !near_vec3(first->transform().position(), glm::vec3{ -1.0f, 0.0f, 0.0f })) {
        return TestResult::fail("local translation should apply delta in the node rotation frame");
    }

    if (!session.scale(scene, glm::vec3{ 2.0f, 3.0f, 4.0f }) ||
        !near_vec3(first->transform().scale(), glm::vec3{ 2.0f, 3.0f, 4.0f }) ||
        !near_quat(first->transform().rotation(), halfTurn)) {
        return TestResult::fail("local scale should multiply scale without changing rotation");
    }

    session.clear();
    if (session.is_active() ||
        session.state() != editor::TransformSessionState::Idle ||
        !session.targets().empty() ||
        session.space() != editor::TransformSpace::World ||
        session.pivot_mode() != editor::TransformPivotMode::SelectionCenter ||
        !near_vec3(session.pivot(), glm::vec3{ 0.0f, 0.0f, 0.0f })) {
        return TestResult::fail("clear should reset TransformSession state and options");
    }

    locked->metadata().locked = true;
    if (session.begin(scene, std::vector<editor::SceneNodeId>{ lockedId }, lockedId, {}) ||
        session.state() != editor::TransformSessionState::Idle) {
        return TestResult::fail("begin should fail when no selectable targets can be captured");
    }

    return TestResult::pass();
}

} // namespace locus::tests
