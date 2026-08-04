/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GizmoTestSuite.h"

#include "editor/gizmo/TransformGizmo.h"

namespace {

[[nodiscard]] locus::editor::TransformGizmoHitTestInput base_input()
{
    locus::editor::TransformGizmoHitTestInput input;
    input.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
    input.ray.origin = glm::vec3{ 0.0f, 0.0f, 3.0f };
    input.ray.direction = glm::vec3{ 0.0f, 0.0f, -1.0f };
    input.viewDirection = glm::vec3{ 0.0f, 0.0f, -1.0f };
    input.visualScale = 1.0f;
    return input;
}

} // namespace

namespace locus::tests {

TestResult run_transform_gizmo_tests()
{
    using namespace editor;

    TransformGizmoConfig config;
    config.axisLength = 2.0f;
    config.axisThickness = 0.08f;
    config.shaftRadius = 0.025f;
    config.arrowLength = 0.24f;
    config.arrowRadius = 0.085f;
    config.planeOffset = 0.2f;
    config.planeSize = 0.4f;
    config.planePickingPadding = 0.03f;
    config.centerRadius = 0.15f;
    config.rotationRadius = 1.0f;
    config.rotationThickness = 0.08f;
    config.rotationTubeRadius = 0.018f;
    config.scaleCubeSize = 0.18f;
    config.pickingPadding = 0.025f;

    TransformGizmo gizmo(config);
    if (gizmo.config().axisLength != 2.0f) {
        return TestResult::fail("TransformGizmo should expose its configured values");
    }

    config.axisLength = 3.0f;
    gizmo.set_config(config);
    if (gizmo.config().axisLength != 3.0f) {
        return TestResult::fail("set_config should replace TransformGizmo configuration");
    }

    TransformGizmoHitTestInput input = base_input();
    input.mode = GizmoMode::None;
    if (gizmo.hit_test(input).is_valid()) {
        return TestResult::fail("hit_test should reject GizmoMode::None");
    }

    input.mode = GizmoMode::Translate;
    GizmoHit hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Translate ||
        hit.axis != GizmoAxis::XYZ) {
        return TestResult::fail("translate hit-test should hit the center free handle");
    }

    input.ray.origin = glm::vec3{ 1.5f, 0.02f, 3.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Translate ||
        hit.axis != GizmoAxis::X) {
        return TestResult::fail("translate hit-test should detect axis segments");
    }

    input.ray.origin = glm::vec3{ 3.6f, 0.0f, 3.0f };
    if (gizmo.hit_test(input).is_valid()) {
        return TestResult::fail("translate hit-test should reject points far beyond the arrow tip");
    }

    input.ray.origin = glm::vec3{ 3.0f, 0.09f, 3.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Translate ||
        hit.axis != GizmoAxis::X) {
        return TestResult::fail("translate hit-test should detect the cone tip volume");
    }

    input.ray.origin = glm::vec3{ 0.3f, 0.3f, 3.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Translate ||
        hit.axis != GizmoAxis::XY) {
        return TestResult::fail("translate hit-test should detect plane handles");
    }

    input.ray.origin = glm::vec3{ 0.18f, 0.18f, 3.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.axis != GizmoAxis::XY) {
        return TestResult::fail("plane hit-test should include configured picking padding");
    }

    input.mode = GizmoMode::Scale;
    input.ray.origin = glm::vec3{ 0.0f, 1.4f, 3.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Scale ||
        hit.axis != GizmoAxis::Y) {
        return TestResult::fail("scale hit-test should reuse axis handles with scale mode");
    }

    input.ray.origin = glm::vec3{ 0.09f, 3.0f, 3.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Scale ||
        hit.axis != GizmoAxis::Y) {
        return TestResult::fail("scale hit-test should detect the cube endpoint volume");
    }

    input.mode = GizmoMode::Rotate;
    input.ray.origin = glm::vec3{ 1.0f, 0.0f, 3.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Rotate ||
        hit.axis != GizmoAxis::Z) {
        return TestResult::fail("rotate hit-test should detect a rotation ring");
    }

    input.ray.origin = glm::vec3{ 3.0f, 1.0f, 0.0f };
    input.ray.direction = glm::vec3{ -1.0f, 0.0f, 0.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Rotate ||
        hit.axis != GizmoAxis::X) {
        return TestResult::fail("rotate hit-test should detect the X rotation ring");
    }

    input.ray.origin = glm::vec3{ 1.0f, 3.0f, 0.0f };
    input.ray.direction = glm::vec3{ 0.0f, -1.0f, 0.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Rotate ||
        hit.axis != GizmoAxis::Y) {
        return TestResult::fail("rotate hit-test should detect the Y rotation ring");
    }

    input.ray.origin = glm::vec3{ config.rotationRadius * config.viewRingScale, 0.0f, 3.0f };
    input.ray.direction = glm::vec3{ 0.0f, 0.0f, -1.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Rotate ||
        hit.axis != GizmoAxis::View) {
        return TestResult::fail("rotate hit-test should detect the view-facing ring");
    }

    input.mode = GizmoMode::Universal;
    input.ray.origin = glm::vec3{ 0.0f, 0.0f, 3.0f };
    input.ray.direction = glm::vec3{ 0.0f, 0.0f, -1.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.axis != GizmoAxis::XYZ) {
        return TestResult::fail("universal hit-test should return the best available sub-gizmo hit");
    }

    input.mode = GizmoMode::Translate;
    input.visualScale = 2.0f;
    input.ray.origin = glm::vec3{ 4.0f, 0.09f, 3.0f };
    hit = gizmo.hit_test(input);
    if (!hit.is_valid() ||
        hit.mode != GizmoMode::Translate ||
        hit.axis != GizmoAxis::X) {
        return TestResult::fail("hit-test should scale handle volumes with visualScale");
    }

    input.ray.origin = glm::vec3{ 8.0f, 8.0f, 3.0f };
    if (gizmo.hit_test(input).is_valid()) {
        return TestResult::fail("hit_test should return an invalid hit when no handle is near the ray");
    }

    return TestResult::pass();
}

} // namespace locus::tests
