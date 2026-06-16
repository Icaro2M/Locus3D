#pragma once

#include <glm/glm.hpp>

namespace locus::kernel::math {

struct Ray {
    glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
    glm::vec3 direction{ 0.0f, 0.0f, -1.0f };

    [[nodiscard]] glm::vec3 at(float distance) const
    {
        return origin + direction * distance;
    }

    [[nodiscard]] Ray normalized() const
    {
        const float length = glm::length(direction);
        if (length <= 0.0f) {
            return *this;
        }

        return Ray{ origin, direction / length };
    }
};

}
