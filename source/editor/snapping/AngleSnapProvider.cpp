/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/snapping/AngleSnapProvider.h"

#include <cmath>
#include <glm/glm.hpp>

namespace locus::editor {
    namespace {

        [[nodiscard]] float snap_scalar(float value, float step)
        {
            return std::round(value / step) * step;
        }

    } // namespace

    SnapMode AngleSnapProvider::mode() const
    {
        return SnapMode::Angle;
    }

    SnapResult AngleSnapProvider::snap(
        const SnapSettings& settings,
        const SnapContext& context) const
    {
        const float step = settings.angle_increment();
        if (step <= 0.0f) {
            return SnapResult::none();
        }

        const glm::vec3 delta = context.candidatePosition - context.referenceOrigin;
        const float radius = glm::length(glm::vec2{ delta.x, delta.y });

        if (radius <= 0.000001f) {
            return SnapResult::none();
        }

        const float angle = std::atan2(delta.y, delta.x);
        const float snappedAngle = snap_scalar(angle, step);

        glm::vec3 snapped = context.candidatePosition;
        snapped.x = context.referenceOrigin.x + std::cos(snappedAngle) * radius;
        snapped.y = context.referenceOrigin.y + std::sin(snappedAngle) * radius;

        const float distance = glm::length(snapped - context.candidatePosition);

        SnapTarget target{};
        target.type = SnapTargetType::Angle;
        target.position = snapped;
        target.normal = { 0.0f, 0.0f, 1.0f };

        return SnapResult::make(
            SnapMode::Angle,
            target,
            context.originalPosition,
            context.candidatePosition,
            snapped,
            distance,
            distance);
    }

} // namespace locus::editor