/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <limits>

#include <glm/glm.hpp>

namespace locus::kernel::math {

/**
 * @brief Axis-aligned bounding box represented by minimum and maximum corners.
 */
struct Bounds {
    /**
     * @brief Minimum corner of the bounds.
     *
     * @note The default value intentionally starts inverted so an empty bounds
     * can grow from the first expanded point.
     */
    glm::vec3 min{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };

    /**
     * @brief Maximum corner of the bounds.
     *
     * @note The default value intentionally starts inverted so an empty bounds
     * can grow from the first expanded point.
     */
    glm::vec3 max{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    };

    /**
     * @brief Creates an invalid empty bounds.
     *
     * @return Empty bounds ready to be expanded by points or other bounds.
     */
    [[nodiscard]] static Bounds empty()
    {
        return {};
    }

    /**
     * @brief Creates bounds from explicit minimum and maximum corners.
     *
     * @param min Minimum corner.
     * @param max Maximum corner.
     * @return Bounds with the provided corners.
     */
    [[nodiscard]] static Bounds from_min_max(const glm::vec3& min, const glm::vec3& max)
    {
        return Bounds{ min, max };
    }

    /**
     * @brief Creates bounds from a center position and full size.
     *
     * @param center Bounds center.
     * @param size Full bounds size on each axis.
     * @return Bounds centered at the given position.
     */
    [[nodiscard]] static Bounds from_center_size(const glm::vec3& center, const glm::vec3& size)
    {
        const glm::vec3 half = size * 0.5f;
        return Bounds{ center - half, center + half };
    }

    /**
     * @brief Checks whether the bounds has ordered minimum and maximum corners.
     *
     * @return True when minimum is less than or equal to maximum on all axes.
     */
    [[nodiscard]] bool is_valid() const
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    /**
     * @brief Computes the bounds center.
     *
     * @return Average position between minimum and maximum corners.
     */
    [[nodiscard]] glm::vec3 center() const
    {
        return (min + max) * 0.5f;
    }

    /**
     * @brief Computes the full bounds size.
     *
     * @return Maximum corner minus minimum corner, or zero for invalid bounds.
     */
    [[nodiscard]] glm::vec3 size() const
    {
        if (!is_valid()) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        return max - min;
    }

    /**
     * @brief Computes half of the bounds size.
     *
     * @return Half extent on each axis.
     */
    [[nodiscard]] glm::vec3 half_extent() const
    {
        return size() * 0.5f;
    }

    /**
     * @brief Checks whether a point lies inside the bounds.
     *
     * @param point Point to test.
     * @return True when the point is inside or on the bounds surface.
     */
    [[nodiscard]] bool contains(const glm::vec3& point) const
    {
        return is_valid()
            && point.x >= min.x && point.x <= max.x
            && point.y >= min.y && point.y <= max.y
            && point.z >= min.z && point.z <= max.z;
    }

    /**
     * @brief Resets this bounds to the empty invalid state.
     */
    void reset()
    {
        *this = Bounds::empty();
    }

    /**
     * @brief Expands this bounds to include a point.
     *
     * @param point Point to include.
     */
    void expand(const glm::vec3& point)
    {
        if (!is_valid()) {
            min = point;
            max = point;
            return;
        }

        min.x = point.x < min.x ? point.x : min.x;
        min.y = point.y < min.y ? point.y : min.y;
        min.z = point.z < min.z ? point.z : min.z;

        max.x = point.x > max.x ? point.x : max.x;
        max.y = point.y > max.y ? point.y : max.y;
        max.z = point.z > max.z ? point.z : max.z;
    }

    /**
     * @brief Expands this bounds to include another bounds.
     *
     * @param bounds Bounds to include.
     */
    void expand(const Bounds& bounds)
    {
        if (!bounds.is_valid()) {
            return;
        }

        expand(bounds.min);
        expand(bounds.max);
    }

    /**
     * @brief Returns an expanded copy of this bounds.
     *
     * @param point Point to include in the returned bounds.
     * @return Expanded bounds copy.
     */
    [[nodiscard]] Bounds expanded(const glm::vec3& point) const
    {
        Bounds result = *this;
        result.expand(point);
        return result;
    }

    /**
     * @brief Transforms the bounds and returns an axis-aligned bounds around the result.
     *
     * @param matrix Transformation matrix applied to all eight corners.
     * @return Axis-aligned bounds enclosing the transformed corners.
     */
    [[nodiscard]] Bounds transformed(const glm::mat4& matrix) const
    {
        if (!is_valid()) {
            return Bounds::empty();
        }

        const std::array<glm::vec3, 8> corners = {
            glm::vec3{ min.x, min.y, min.z },
            glm::vec3{ max.x, min.y, min.z },
            glm::vec3{ min.x, max.y, min.z },
            glm::vec3{ max.x, max.y, min.z },
            glm::vec3{ min.x, min.y, max.z },
            glm::vec3{ max.x, min.y, max.z },
            glm::vec3{ min.x, max.y, max.z },
            glm::vec3{ max.x, max.y, max.z }
        };

        Bounds result = Bounds::empty();
        for (const glm::vec3& corner : corners) {
            const glm::vec4 transformedCorner = matrix * glm::vec4{ corner, 1.0f };
            result.expand(glm::vec3{ transformedCorner });
        }

        return result;
    }
};

}
