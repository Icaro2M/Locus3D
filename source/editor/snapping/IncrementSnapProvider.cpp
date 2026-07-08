/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/snapping/IncrementSnapProvider.h"

#include <cmath>
#include <glm/glm.hpp>

namespace locus::editor {
    namespace {

        [[nodiscard]] float snap_scalar(float value, float step)
        {
            return std::round(value / step) * step;
        }

        [[nodiscard]] glm::vec3 snap_delta(const glm::vec3& delta, float step)
        {
            return {
                snap_scalar(delta.x, step),
                snap_scalar(delta.y, step),
                snap_scalar(delta.z, step)
            };
        }

    } // namespace

    SnapMode IncrementSnapProvider::mode() const
    {
        return SnapMode::Increment;
    }

    SnapResult IncrementSnapProvider::snap(
        const SnapSettings& settings,
        const SnapContext& context) const
    {
        const float step = settings.linear_increment();
        if (step <= 0.0f) {
            return SnapResult::none();
        }

        const glm::vec3 delta = context.candidatePosition - context.referenceOrigin;
        const glm::vec3 snappedDelta = snap_delta(delta, step);
        const glm::vec3 snapped = context.referenceOrigin + snappedDelta;
        const float distance = glm::length(snapped - context.candidatePosition);

        SnapTarget target{};
        target.type = SnapTargetType::Increment;
        target.position = snapped;

        return SnapResult::make(
            SnapMode::Increment,
            target,
            context.originalPosition,
            context.candidatePosition,
            snapped,
            distance,
            distance);
    }

} // namespace locus::editor