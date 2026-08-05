/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/core/ToolEvent.h"

#include <algorithm>

#include <glm/vec2.hpp>

namespace locus::editor {

    enum class SelectionShapeKind {
        Point,
        Box,
        Circle,
        Lasso
    };

    enum class SelectionContainment {
        FullyContained,
        Intersecting
    };

    enum class SelectionDepthMode {
        VisibleOnly,
        Through
    };

    enum class SelectionOperation {
        Replace,
        Add,
        Subtract,
        Toggle
    };

    struct ScreenSelectionRect {
        glm::vec2 min{ 0.0f, 0.0f };
        glm::vec2 max{ 0.0f, 0.0f };

        [[nodiscard]] static ScreenSelectionRect from_points(
            const glm::vec2& a,
            const glm::vec2& b)
        {
            return ScreenSelectionRect{
                glm::vec2{
                    std::min(a.x, b.x),
                    std::min(a.y, b.y) },
                glm::vec2{
                    std::max(a.x, b.x),
                    std::max(a.y, b.y) }
            };
        }

        [[nodiscard]] bool empty() const
        {
            return width() <= 0.0f || height() <= 0.0f;
        }

        [[nodiscard]] float width() const
        {
            return max.x - min.x;
        }

        [[nodiscard]] float height() const
        {
            return max.y - min.y;
        }

        [[nodiscard]] bool contains(const glm::vec2& point) const
        {
            return point.x >= min.x && point.x <= max.x &&
                point.y >= min.y && point.y <= max.y;
        }

        [[nodiscard]] bool intersects_segment(
            const glm::vec2& a,
            const glm::vec2& b) const
        {
            if (contains(a) || contains(b)) {
                return true;
            }

            const auto orientation = [](
                const glm::vec2& p,
                const glm::vec2& q,
                const glm::vec2& r) {
                const float value =
                    (q.y - p.y) * (r.x - q.x) -
                    (q.x - p.x) * (r.y - q.y);
                if (value == 0.0f) {
                    return 0;
                }
                return value > 0.0f ? 1 : 2;
            };

            const auto onSegment = [](
                const glm::vec2& p,
                const glm::vec2& q,
                const glm::vec2& r) {
                return q.x <= std::max(p.x, r.x) &&
                    q.x >= std::min(p.x, r.x) &&
                    q.y <= std::max(p.y, r.y) &&
                    q.y >= std::min(p.y, r.y);
            };

            const auto intersects = [&](const glm::vec2& p1,
                const glm::vec2& q1,
                const glm::vec2& p2,
                const glm::vec2& q2) {
                const int o1 = orientation(p1, q1, p2);
                const int o2 = orientation(p1, q1, q2);
                const int o3 = orientation(p2, q2, p1);
                const int o4 = orientation(p2, q2, q1);

                if (o1 != o2 && o3 != o4) {
                    return true;
                }

                return (o1 == 0 && onSegment(p1, p2, q1)) ||
                    (o2 == 0 && onSegment(p1, q2, q1)) ||
                    (o3 == 0 && onSegment(p2, p1, q2)) ||
                    (o4 == 0 && onSegment(p2, q1, q2));
            };

            const glm::vec2 topLeft{ min.x, min.y };
            const glm::vec2 topRight{ max.x, min.y };
            const glm::vec2 bottomRight{ max.x, max.y };
            const glm::vec2 bottomLeft{ min.x, max.y };

            return intersects(a, b, topLeft, topRight) ||
                intersects(a, b, topRight, bottomRight) ||
                intersects(a, b, bottomRight, bottomLeft) ||
                intersects(a, b, bottomLeft, topLeft);
        }

        [[nodiscard]] ScreenSelectionRect clipped(
            const glm::vec2& viewportSize) const
        {
            if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f) {
                return {};
            }

            ScreenSelectionRect result = *this;
            result.min.x = std::clamp(result.min.x, 0.0f, viewportSize.x - 1.0f);
            result.min.y = std::clamp(result.min.y, 0.0f, viewportSize.y - 1.0f);
            result.max.x = std::clamp(result.max.x, 0.0f, viewportSize.x - 1.0f);
            result.max.y = std::clamp(result.max.y, 0.0f, viewportSize.y - 1.0f);
            return result;
        }
    };

    [[nodiscard]] inline SelectionOperation selection_operation_from_modifiers(
        const ToolModifiers modifiers)
    {
        const bool additive = has_modifier(modifiers, ToolModifiers::Additive);
        const bool toggle = has_modifier(modifiers, ToolModifiers::Toggle);

        if (additive && toggle) {
            return SelectionOperation::Subtract;
        }

        if (toggle) {
            return SelectionOperation::Toggle;
        }

        if (additive) {
            return SelectionOperation::Add;
        }

        return SelectionOperation::Replace;
    }

} // namespace locus::editor
