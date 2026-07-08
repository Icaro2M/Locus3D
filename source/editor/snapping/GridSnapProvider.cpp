/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/snapping/GridSnapProvider.h"

#include <cmath>
#include <glm/glm.hpp>

namespace locus::editor {
    namespace {

        [[nodiscard]] float snap_scalar(float value, float step)
        {
            return std::round(value / step) * step;
        }

        [[nodiscard]] glm::vec3 snap_vec3(const glm::vec3& value, float step)
        {
            return {
                snap_scalar(value.x, step),
                snap_scalar(value.y, step),
                snap_scalar(value.z, step)
            };
        }

    } // namespace

    SnapMode GridSnapProvider::mode() const
    {
        return SnapMode::Grid;
    }

    SnapResult GridSnapProvider::snap(
        const SnapSettings& settings,
        const SnapContext& context) const
    {
        const float step = settings.grid_size();
        if (step <= 0.0f) {
            return SnapResult::none();
        }

        const glm::vec3 snapped = snap_vec3(context.candidatePosition, step);
        const float distance = glm::length(snapped - context.candidatePosition);

        SnapTarget target{};
        target.type = SnapTargetType::GridPoint;
        target.position = snapped;

        return SnapResult::make(
            SnapMode::Grid,
            target,
            context.originalPosition,
            context.candidatePosition,
            snapped,
            distance,
            distance);
    }

} // namespace locus::editor