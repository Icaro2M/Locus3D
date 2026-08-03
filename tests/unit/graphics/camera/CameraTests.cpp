/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/camera/Camera.h"
#include "graphics/camera/CameraRayBuilder.h"
#include "graphics/common/GraphicsTypes.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <iostream>

namespace {

[[nodiscard]] bool near(
    float lhs,
    float rhs,
    float epsilon = 0.0001f)
{
    return std::abs(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool near_vec3(
    const glm::vec3& lhs,
    const glm::vec3& rhs,
    float epsilon = 0.0001f)
{
    return glm::length(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool test_orthographic_projection_matrix()
{
    locus::graphics::Projection projection{};
    projection.set_orthographic(8.0f, 2.0f, 0.01f, 100.0f);

    const glm::mat4 expected =
        glm::ortho(-8.0f, 8.0f, -4.0f, 4.0f, 0.01f, 100.0f);
    const glm::mat4 actual = projection.matrix();

    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            if (!near(actual[column][row], expected[column][row])) {
                return false;
            }
        }
    }

    projection.set_orthographic(-1.0f, 0.0f, 0.01f, 100.0f);
    return near(
        projection.orthographic_height(),
        locus::graphics::Projection::min_orthographic_height())
        && near(projection.aspect_ratio(), 0.0001f);
}

[[nodiscard]] bool test_orthographic_resize_keeps_height()
{
    locus::graphics::Projection projection{};
    projection.set_orthographic(6.0f, 1.0f, 0.01f, 100.0f);
    projection.set_aspect_ratio(3.0f);

    return near(projection.orthographic_height(), 6.0f)
        && near(projection.aspect_ratio(), 3.0f);
}

[[nodiscard]] bool test_projection_switch_framing_formula()
{
    constexpr float distance = 7.0f;
    constexpr float fov = 0.78539816339f;
    const float orthographicHeight =
        2.0f * distance * std::tan(fov * 0.5f);
    const float restoredDistance =
        orthographicHeight / (2.0f * std::tan(fov * 0.5f));

    return near(restoredDistance, distance);
}

[[nodiscard]] bool test_orthographic_camera_rays()
{
    locus::graphics::Camera camera{};
    camera.look_at(
        { 0.0f, 0.0f, 5.0f },
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f });
    camera.projection().set_orthographic(4.0f, 1.0f, 0.01f, 100.0f);

    const locus::graphics::ViewportRect viewport{
        0,
        0,
        400,
        400
    };

    const locus::graphics::CameraRay center =
        locus::graphics::CameraRayBuilder::from_viewport_pixel(
            camera,
            viewport,
            200.0f,
            200.0f);
    const locus::graphics::CameraRay right =
        locus::graphics::CameraRayBuilder::from_viewport_pixel(
            camera,
            viewport,
            300.0f,
            200.0f);

    return glm::length(center.origin - right.origin) > 0.1f
        && near_vec3(center.direction, right.direction)
        && glm::dot(center.direction, camera.forward()) > 0.999f;
}

} // namespace

bool run_camera_tests()
{
    struct TestCase {
        const char* name = "";
        bool (*run)() = nullptr;
    };

    const TestCase tests[] = {
        { "OrthographicProjectionMatrix", &test_orthographic_projection_matrix },
        { "OrthographicResizeKeepsHeight", &test_orthographic_resize_keeps_height },
        { "ProjectionSwitchFramingFormula", &test_projection_switch_framing_formula },
        { "OrthographicCameraRays", &test_orthographic_camera_rays },
    };

    for (const TestCase& test : tests) {
        if (!test.run()) {
            std::cerr << test.name << ": failed\n";
            return false;
        }
    }

    return true;
}
