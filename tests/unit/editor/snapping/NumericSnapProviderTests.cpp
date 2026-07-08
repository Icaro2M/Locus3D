/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SnappingTestSuite.h"

#include "editor/snapping/AngleSnapProvider.h"
#include "editor/snapping/GridSnapProvider.h"
#include "editor/snapping/IncrementSnapProvider.h"

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

} // namespace

namespace locus::tests {

TestResult run_numeric_snap_provider_tests()
{
    using namespace editor;

    SnapSettings settings;
    settings.set_modes(SnapMode::Grid | SnapMode::Increment | SnapMode::Angle);

    SnapContext context;
    context.originalPosition = glm::vec3{ 0.0f, 0.0f, 0.0f };
    context.candidatePosition = glm::vec3{ 1.24f, -0.26f, 0.76f };

    GridSnapProvider grid;
    settings.set_grid_size(0.5f);
    const SnapResult gridResult = grid.snap(settings, context);
    if (grid.mode() != SnapMode::Grid ||
        !grid.is_enabled(settings, context) ||
        !gridResult.is_valid() ||
        gridResult.target.type != SnapTargetType::GridPoint ||
        !near_vec3(gridResult.snappedPosition, glm::vec3{ 1.0f, -0.5f, 1.0f }) ||
        !near_vec3(gridResult.target.position, gridResult.snappedPosition) ||
        !near(gridResult.distance, glm::length(gridResult.snappedPosition - context.candidatePosition))) {
        return TestResult::fail("GridSnapProvider should snap each coordinate to the nearest grid point");
    }

    IncrementSnapProvider increment;
    settings.set_linear_increment(0.25f);
    context.referenceOrigin = glm::vec3{ 1.0f, 1.0f, 1.0f };
    context.candidatePosition = glm::vec3{ 1.36f, 1.61f, 0.63f };
    const SnapResult incrementResult = increment.snap(settings, context);
    if (increment.mode() != SnapMode::Increment ||
        !incrementResult.is_valid() ||
        incrementResult.target.type != SnapTargetType::Increment ||
        !near_vec3(incrementResult.snappedPosition, glm::vec3{ 1.25f, 1.5f, 0.75f })) {
        return TestResult::fail("IncrementSnapProvider should snap deltas around the reference origin");
    }

    AngleSnapProvider angle;
    settings.set_angle_increment(1.57079632679f);
    context.referenceOrigin = glm::vec3{ 0.0f, 0.0f, 0.0f };
    context.candidatePosition = glm::vec3{ 1.0f, 1.0f, 2.0f };
    const SnapResult angleResult = angle.snap(settings, context);
    if (angle.mode() != SnapMode::Angle ||
        !angleResult.is_valid() ||
        angleResult.target.type != SnapTargetType::Angle ||
        !near_vec3(angleResult.snappedPosition, glm::vec3{ 0.0f, 1.41421356f, 2.0f }) ||
        !near_vec3(angleResult.target.normal, glm::vec3{ 0.0f, 0.0f, 1.0f })) {
        return TestResult::fail("AngleSnapProvider should snap around the reference origin in the XY plane");
    }

    context.candidatePosition = glm::vec3{ 0.0f, 0.0f, 2.0f };
    if (angle.snap(settings, context).is_valid()) {
        return TestResult::fail("AngleSnapProvider should reject zero-radius angular snaps");
    }

    settings.disable(SnapMode::Grid);
    if (grid.is_enabled(settings, context)) {
        return TestResult::fail("ISnapProvider default enable check should honor SnapSettings modes");
    }

    return TestResult::pass();
}

} // namespace locus::tests
