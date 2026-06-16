#pragma once

#include <array>
#include <limits>

#include <glm/glm.hpp>

namespace locus::kernel::math {

struct Bounds {
    glm::vec3 min{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };

    glm::vec3 max{
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    };

    [[nodiscard]] static Bounds empty()
    {
        return {};
    }

    [[nodiscard]] static Bounds from_min_max(const glm::vec3& min, const glm::vec3& max)
    {
        return Bounds{ min, max };
    }

    [[nodiscard]] static Bounds from_center_size(const glm::vec3& center, const glm::vec3& size)
    {
        const glm::vec3 half = size * 0.5f;
        return Bounds{ center - half, center + half };
    }

    [[nodiscard]] bool is_valid() const
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    [[nodiscard]] glm::vec3 center() const
    {
        return (min + max) * 0.5f;
    }

    [[nodiscard]] glm::vec3 size() const
    {
        if (!is_valid()) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        return max - min;
    }

    [[nodiscard]] glm::vec3 half_extent() const
    {
        return size() * 0.5f;
    }

    [[nodiscard]] bool contains(const glm::vec3& point) const
    {
        return is_valid()
            && point.x >= min.x && point.x <= max.x
            && point.y >= min.y && point.y <= max.y
            && point.z >= min.z && point.z <= max.z;
    }

    void reset()
    {
        *this = Bounds::empty();
    }

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

    void expand(const Bounds& bounds)
    {
        if (!bounds.is_valid()) {
            return;
        }

        expand(bounds.min);
        expand(bounds.max);
    }

    [[nodiscard]] Bounds expanded(const glm::vec3& point) const
    {
        Bounds result = *this;
        result.expand(point);
        return result;
    }

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
