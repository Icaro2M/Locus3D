/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/scene/NodeTransform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::editor {

    const glm::vec3& NodeTransform::position() const
    {
        return position_;
    }

    const glm::quat& NodeTransform::rotation() const
    {
        return rotation_;
    }

    const glm::vec3& NodeTransform::scale() const
    {
        return scale_;
    }

    void NodeTransform::set_position(const glm::vec3& position)
    {
        position_ = position;
    }

    void NodeTransform::set_rotation(const glm::quat& rotation)
    {
        rotation_ = glm::normalize(rotation);
    }

    void NodeTransform::set_scale(const glm::vec3& scale)
    {
        scale_ = scale;
    }

    void NodeTransform::translate(const glm::vec3& offset)
    {
        position_ += offset;
    }

    void NodeTransform::reset()
    {
        position_ = glm::vec3{ 0.0f, 0.0f, 0.0f };
        rotation_ = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        scale_ = glm::vec3{ 1.0f, 1.0f, 1.0f };
    }

    glm::mat4 NodeTransform::matrix() const
    {
        const glm::mat4 translationMatrix =
            glm::translate(glm::mat4{ 1.0f }, position_);

        const glm::mat4 rotationMatrix =
            glm::mat4_cast(rotation_);

        const glm::mat4 scaleMatrix =
            glm::scale(glm::mat4{ 1.0f }, scale_);

        return translationMatrix * rotationMatrix * scaleMatrix;
    }

}