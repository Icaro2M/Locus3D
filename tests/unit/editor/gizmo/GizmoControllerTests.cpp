/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GizmoTestSuite.h"

#include "editor/gizmo/GizmoController.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"
#include "editor/selection/SelectionState.h"

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

[[nodiscard]] locus::editor::GizmoPointerInput pointer_at(float x, float y, float z = 3.0f)
{
    locus::editor::GizmoPointerInput pointer;
    pointer.ray.origin = glm::vec3{ x, y, z };
    pointer.ray.direction = glm::vec3{ 0.0f, 0.0f, -1.0f };
    pointer.viewDirection = glm::vec3{ 0.0f, 0.0f, -1.0f };
    pointer.viewRight = glm::vec3{ 1.0f, 0.0f, 0.0f };
    pointer.viewUp = glm::vec3{ 0.0f, 1.0f, 0.0f };
    pointer.visualScale = 1.0f;
    return pointer;
}

} // namespace

namespace locus::tests {

TestResult run_gizmo_controller_tests()
{
    using namespace editor;

    GizmoController controller;

    GizmoHoverInput hover;
    hover.mode = GizmoMode::Translate;
    hover.pointer = pointer_at(0.0f, 0.0f);

    GizmoHit hoverHit = controller.update_hover(hover);
    if (!hoverHit.is_valid() ||
        hoverHit.mode != GizmoMode::Translate ||
        hoverHit.axis != GizmoAxis::XYZ ||
        !controller.state().hovered.is_valid()) {
        return TestResult::fail("update_hover should update state with the hit-tested handle");
    }

    controller.state().visible = false;
    hoverHit = controller.update_hover(hover);
    if (hoverHit.is_valid() || controller.state().hovered.is_valid()) {
        return TestResult::fail("update_hover should clear hover state when the gizmo cannot interact");
    }

    controller.clear();
    if (controller.state().dragging || controller.session().is_active()) {
        return TestResult::fail("clear should reset controller state and session");
    }

    EditorScene scene;
    const SceneNodeId nodeId = scene.create_empty("Dragged");
    SceneNode* node = scene.find_node(nodeId);
    if (!node) {
        return TestResult::fail("test scene should contain a draggable node");
    }

    node->transform().set_position(glm::vec3{ 0.0f, 0.0f, 0.0f });

    GizmoBeginDragTargetsInput begin;
    begin.scene = &scene;
    begin.targets = { nodeId };
    begin.active = nodeId;
    begin.mode = GizmoMode::Translate;
    begin.pointer = pointer_at(1.0f, 0.0f);

    GizmoControllerResult result = controller.begin_drag(begin);
    if (!result.success ||
        controller.state().active.axis != GizmoAxis::X ||
        !controller.state().dragging ||
        !controller.session().is_active()) {
        return TestResult::fail("begin_drag should start a transform session from a valid axis hit");
    }

    GizmoDragInput drag;
    drag.pointer = pointer_at(2.0f, 0.0f);
    result = controller.update_drag(scene, drag);
    if (!result.success ||
        !result.changed ||
        !result.constraint.is_valid() ||
        !near_vec3(node->transform().position(), glm::vec3{ 1.0f, 0.0f, 0.0f })) {
        return TestResult::fail("update_drag should apply incremental translation preview");
    }

    drag.pointer = pointer_at(3.0f, 0.0f);
    result = controller.update_drag(scene, drag);
    if (!result.success ||
        !result.changed ||
        !near_vec3(node->transform().position(), glm::vec3{ 2.0f, 0.0f, 0.0f })) {
        return TestResult::fail("update_drag should convert absolute constraint to incremental scene changes");
    }

    if (!controller.end_drag() ||
        controller.state().dragging ||
        controller.session().is_active() ||
        !near_vec3(node->transform().position(), glm::vec3{ 2.0f, 0.0f, 0.0f })) {
        return TestResult::fail("end_drag should confirm the preview transform");
    }

    begin.pointer = pointer_at(1.0f, 0.0f);
    result = controller.begin_drag(begin);
    if (!result.success) {
        return TestResult::fail("begin_drag should allow a second drag after confirmation");
    }

    drag.pointer = pointer_at(2.0f, 0.0f);
    result = controller.update_drag(scene, drag);
    if (!result.success ||
        !near_vec3(node->transform().position(), glm::vec3{ 3.0f, 0.0f, 0.0f })) {
        return TestResult::fail("second drag should preview from the current node transform");
    }

    if (!controller.cancel_drag(scene) ||
        controller.state().dragging ||
        controller.session().is_active() ||
        !near_vec3(node->transform().position(), glm::vec3{ 2.0f, 0.0f, 0.0f })) {
        return TestResult::fail("cancel_drag should restore transforms captured at drag start");
    }

    GizmoBeginDragInput missingSelection;
    missingSelection.scene = &scene;
    result = controller.begin_drag(missingSelection);
    if (result.success) {
        return TestResult::fail("begin_drag from selection should reject missing SelectionState");
    }

    return TestResult::pass();
}

} // namespace locus::tests
