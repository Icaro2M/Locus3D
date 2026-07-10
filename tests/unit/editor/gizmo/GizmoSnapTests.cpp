/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GizmoTestSuite.h"

#include "editor/gizmo/GizmoSnap.h"
#include "editor/snapping/ISnapProvider.h"
#include "editor/snapping/SnapSolver.h"

#include <cmath>
#include <memory>

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

class FixedSnapProvider final : public locus::editor::ISnapProvider {
public:
    explicit FixedSnapProvider(const glm::vec3& snapped)
        : snapped_(snapped)
    {
    }

    [[nodiscard]] locus::editor::SnapMode mode() const override
    {
        return locus::editor::SnapMode::Grid;
    }

    [[nodiscard]] locus::editor::SnapResult snap(
        const locus::editor::SnapSettings& settings,
        const locus::editor::SnapContext& context) const override
    {
        (void)settings;

        locus::editor::SnapTarget target;
        target.type = locus::editor::SnapTargetType::GridPoint;
        target.position = snapped_;

        return locus::editor::SnapResult::make(
            locus::editor::SnapMode::Grid,
            target,
            context.originalPosition,
            context.candidatePosition,
            snapped_,
            glm::length(snapped_ - context.candidatePosition),
            1.0f);
    }

private:
    glm::vec3 snapped_{ 0.0f, 0.0f, 0.0f };
};

} // namespace

namespace locus::tests {

TestResult run_gizmo_snap_tests()
{
    using namespace editor;

    GizmoSnapRequest request;
    request.originalPosition = glm::vec3{ 1.0f, 2.0f, 3.0f };
    request.candidatePosition = glm::vec3{ 2.0f, 4.0f, 6.0f };

    const GizmoSnapResult unsnapped = GizmoSnap::snap_position(request);
    if (unsnapped.snapped ||
        !near_vec3(unsnapped.snappedPosition, request.candidatePosition) ||
        !near_vec3(unsnapped.delta, glm::vec3{ 1.0f, 2.0f, 3.0f })) {
        return TestResult::fail("snap_position should pass through when settings or solver are missing");
    }

    SnapSettings settings;
    settings.set_modes(SnapMode::Grid);
    settings.set_max_distance(10.0f);

    SnapSolver solver;
    if (!solver.register_provider(std::make_unique<FixedSnapProvider>(glm::vec3{ 4.0f, 5.0f, 6.0f }))) {
        return TestResult::fail("test snap provider should register");
    }

    request.settings = &settings;
    request.solver = &solver;
    const GizmoSnapResult snapped = GizmoSnap::snap_position(request);
    if (!snapped.snapped ||
        !snapped.source.is_valid() ||
        !near_vec3(snapped.snappedPosition, glm::vec3{ 4.0f, 5.0f, 6.0f }) ||
        !near_vec3(snapped.delta, glm::vec3{ 3.0f, 3.0f, 3.0f })) {
        return TestResult::fail("snap_position should apply a valid SnapSolver result");
    }

    GizmoConstraintResult constraint;
    constraint.valid = true;
    constraint.translation = glm::vec3{ 0.5f, 0.5f, 0.5f };

    request.originalPosition = glm::vec3{ 10.0f, 10.0f, 10.0f };
    const GizmoSnapResult snappedTranslation = GizmoSnap::snap_translation(request, constraint);
    if (!snappedTranslation.snapped ||
        !near_vec3(snappedTranslation.delta, glm::vec3{ -6.0f, -5.0f, -4.0f })) {
        return TestResult::fail("snap_translation should snap original plus constraint translation");
    }

    settings.enable(SnapMode::Angle);
    settings.set_angle_increment(0.25f);
    if (!near(GizmoSnap::snap_angle(0.37f, settings), 0.25f)) {
        return TestResult::fail("snap_angle should round to the configured angle increment");
    }

    settings.disable(SnapMode::Angle);
    if (!near(GizmoSnap::snap_angle(0.37f, settings), 0.37f)) {
        return TestResult::fail("snap_angle should pass through when angle snapping is disabled");
    }

    return TestResult::pass();
}

} // namespace locus::tests
