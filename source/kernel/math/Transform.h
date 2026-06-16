#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::kernel::math {

struct Transform {
    glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
    glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    [[nodiscard]] static Transform identity()
    {
        return {};
    }

    [[nodiscard]] glm::mat4 matrix() const
    {
        const glm::mat4 translationMatrix = glm::translate(glm::mat4{ 1.0f }, translation);
        const glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
        const glm::mat4 scaleMatrix = glm::scale(glm::mat4{ 1.0f }, scale);
        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    [[nodiscard]] glm::mat4 inverse_matrix() const
    {
        const glm::vec3 safeScale{
            scale.x == 0.0f ? 1.0f : scale.x,
            scale.y == 0.0f ? 1.0f : scale.y,
            scale.z == 0.0f ? 1.0f : scale.z
        };

        const glm::mat4 inverseScale = glm::scale(
            glm::mat4{ 1.0f },
            glm::vec3{ 1.0f / safeScale.x, 1.0f / safeScale.y, 1.0f / safeScale.z }
        );
        const glm::mat4 inverseRotation = glm::mat4_cast(glm::inverse(rotation));
        const glm::mat4 inverseTranslation = glm::translate(glm::mat4{ 1.0f }, -translation);

        return inverseScale * inverseRotation * inverseTranslation;
    }

    [[nodiscard]] glm::vec3 transform_point(const glm::vec3& point) const
    {
        return glm::vec3{ matrix() * glm::vec4{ point, 1.0f } };
    }

    [[nodiscard]] glm::vec3 transform_vector(const glm::vec3& vector) const
    {
        return glm::vec3{ matrix() * glm::vec4{ vector, 0.0f } };
    }
};

}
