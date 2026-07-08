/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SnappingTestSuite.h"

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
    FixedSnapProvider(
        locus::editor::SnapMode mode,
        const glm::vec3& snapped,
        float distance,
        float score,
        bool valid = true)
        : mode_(mode)
        , snapped_(snapped)
        , distance_(distance)
        , score_(score)
        , valid_(valid)
    {
    }

    [[nodiscard]] locus::editor::SnapMode mode() const override
    {
        return mode_;
    }

    [[nodiscard]] locus::editor::SnapResult snap(
        const locus::editor::SnapSettings& settings,
        const locus::editor::SnapContext& context) const override
    {
        (void)settings;

        if (!valid_) {
            return locus::editor::SnapResult::none();
        }

        locus::editor::SnapTarget target{};
        target.type = locus::editor::SnapTargetType::GridPoint;
        target.position = snapped_;

        return locus::editor::SnapResult::make(
            mode_,
            target,
            context.originalPosition,
            context.candidatePosition,
            snapped_,
            distance_,
            score_);
    }

private:
    locus::editor::SnapMode mode_ = locus::editor::SnapMode::None;
    glm::vec3 snapped_{ 0.0f, 0.0f, 0.0f };
    float distance_ = 0.0f;
    float score_ = 0.0f;
    bool valid_ = true;
};

} // namespace

namespace locus::tests {

TestResult run_snap_settings_and_solver_tests()
{
    using namespace editor;

    SnapMode mode = SnapMode::Grid;
    mode |= SnapMode::Vertex;
    if (!has_snap_mode(mode, SnapMode::Grid) ||
        !has_snap_mode(mode, SnapMode::Vertex) ||
        has_snap_mode(mode, SnapMode::Face) ||
        ((SnapMode::Grid | SnapMode::Edge) & SnapMode::Vertex) != SnapMode::None) {
        return TestResult::fail("SnapMode operators should combine and test mode masks");
    }

    SnapSettings settings;
    if (!settings.snapping_enabled() ||
        !settings.is_enabled(SnapMode::Grid) ||
        !settings.is_enabled(SnapMode::Increment) ||
        !settings.is_enabled(SnapMode::Angle) ||
        settings.is_enabled(SnapMode::Vertex)) {
        return TestResult::fail("SnapSettings should expose default enabled modes");
    }

    settings.enable(SnapMode::Vertex);
    settings.disable(SnapMode::Grid);
    if (!settings.is_enabled(SnapMode::Vertex) ||
        settings.is_enabled(SnapMode::Grid)) {
        return TestResult::fail("SnapSettings enable and disable should update the mode mask");
    }

    settings.set_snapping_enabled(false);
    if (settings.is_enabled(SnapMode::Vertex)) {
        return TestResult::fail("SnapSettings should suppress individual modes when snapping is disabled");
    }
    settings.set_snapping_enabled(true);

    settings.set_grid_size(-1.0f);
    settings.set_linear_increment(0.0f);
    settings.set_angle_increment(-2.0f);
    settings.set_max_distance(-3.0f);
    if (!near(settings.grid_size(), 0.0001f) ||
        !near(settings.linear_increment(), 0.0001f) ||
        !near(settings.angle_increment(), 0.0001f) ||
        !near(settings.max_distance(), 0.0f)) {
        return TestResult::fail("SnapSettings numeric setters should clamp invalid values");
    }

    SnapTarget emptyTarget;
    SnapTarget filledTarget;
    filledTarget.type = SnapTargetType::Vertex;
    if (emptyTarget.is_valid() || !filledTarget.is_valid()) {
        return TestResult::fail("SnapTarget validity should depend on target type");
    }

    const SnapResult none = SnapResult::none();
    const SnapResult made = SnapResult::make(
        SnapMode::Vertex,
        filledTarget,
        glm::vec3{ 1.0f },
        glm::vec3{ 2.0f },
        glm::vec3{ 3.0f },
        4.0f,
        5.0f);
    if (none.is_valid() ||
        !made.is_valid() ||
        made.mode != SnapMode::Vertex ||
        made.target.type != SnapTargetType::Vertex ||
        !near_vec3(made.snappedPosition, glm::vec3{ 3.0f }) ||
        !near(made.distance, 4.0f) ||
        !near(made.score, 5.0f)) {
        return TestResult::fail("SnapResult factories should populate valid and invalid results");
    }

    SnapContext context;
    context.candidatePosition = glm::vec3{ 0.0f, 0.0f, 0.0f };
    if (!near(context.effective_max_distance(2.0f), 2.0f)) {
        return TestResult::fail("SnapContext should use fallback max distance by default");
    }
    context.maxDistanceOverride = 0.25f;
    if (!near(context.effective_max_distance(2.0f), 0.25f)) {
        return TestResult::fail("SnapContext should prefer non-negative max distance overrides");
    }

    SnapSolver solver;
    if (solver.register_provider(nullptr) ||
        solver.provider_count() != 0) {
        return TestResult::fail("SnapSolver should reject null providers");
    }

    settings.set_modes(SnapMode::Grid | SnapMode::Vertex | SnapMode::Edge);
    settings.set_max_distance(0.6f);
    context.maxDistanceOverride = -1.0f;
    if (!solver.register_provider(std::make_unique<FixedSnapProvider>(
            SnapMode::Grid,
            glm::vec3{ 1.0f, 0.0f, 0.0f },
            0.4f,
            10.0f)) ||
        !solver.register_provider(std::make_unique<FixedSnapProvider>(
            SnapMode::Vertex,
            glm::vec3{ 2.0f, 0.0f, 0.0f },
            0.5f,
            1.0f)) ||
        !solver.register_provider(std::make_unique<FixedSnapProvider>(
            SnapMode::Edge,
            glm::vec3{ 3.0f, 0.0f, 0.0f },
            0.7f,
            0.1f)) ||
        !solver.register_provider(std::make_unique<FixedSnapProvider>(
            SnapMode::Grid,
            glm::vec3{ 4.0f, 0.0f, 0.0f },
            0.1f,
            0.1f,
            false)) ||
        solver.provider_count() != 4) {
        return TestResult::fail("SnapSolver should store non-null providers");
    }

    const SnapResult best = solver.solve(settings, context);
    if (!best.is_valid() ||
        best.mode != SnapMode::Vertex ||
        !near_vec3(best.snappedPosition, glm::vec3{ 2.0f, 0.0f, 0.0f })) {
        return TestResult::fail("SnapSolver should choose the lowest score within max distance");
    }

    context.maxDistanceOverride = 0.45f;
    const SnapResult distanceFiltered = solver.solve(settings, context);
    if (!distanceFiltered.is_valid() ||
        distanceFiltered.mode != SnapMode::Grid ||
        !near_vec3(distanceFiltered.snappedPosition, glm::vec3{ 1.0f, 0.0f, 0.0f })) {
        return TestResult::fail("SnapSolver should honor context max distance overrides");
    }

    settings.set_snapping_enabled(false);
    if (solver.solve(settings, context).is_valid()) {
        return TestResult::fail("SnapSolver should return no result when snapping is globally disabled");
    }

    solver.clear();
    if (solver.provider_count() != 0) {
        return TestResult::fail("SnapSolver clear should remove providers");
    }

    return TestResult::pass();
}

} // namespace locus::tests
