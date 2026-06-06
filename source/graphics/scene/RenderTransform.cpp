/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/scene/RenderTransform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::graphics
{
    glm::mat4 RenderTransform::matrix() const
    {
        const glm::mat4 translationMatrix = glm::translate(glm::mat4{ 1.0f }, position);
        const glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
        const glm::mat4 scaleMatrix = glm::scale(glm::mat4{ 1.0f }, scale);

        // Compose TRS in the conventional model-matrix order for column-vector math.
        return translationMatrix * rotationMatrix * scaleMatrix;
    }
}
