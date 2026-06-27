/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace locus::editor {

    /**
     * @brief Local transform stored by an editor scene node.
     */
    class NodeTransform {
    public:
        /**
         * @brief Returns the local translation.
         *
         * @return Local position.
         */
        [[nodiscard]] const glm::vec3& position() const;

        /**
         * @brief Returns the local orientation.
         *
         * @return Local rotation.
         */
        [[nodiscard]] const glm::quat& rotation() const;

        /**
         * @brief Returns the local non-uniform scale.
         *
         * @return Local scale.
         */
        [[nodiscard]] const glm::vec3& scale() const;

        /**
         * @brief Changes the local translation.
         *
         * @param position New local position.
         */
        void set_position(const glm::vec3& position);

        /**
         * @brief Changes the local orientation.
         *
         * @param rotation New local rotation.
         */
        void set_rotation(const glm::quat& rotation);

        /**
         * @brief Changes the local non-uniform scale.
         *
         * @param scale New local scale.
         */
        void set_scale(const glm::vec3& scale);

        /**
         * @brief Adds an offset to the local translation.
         *
         * @param offset Translation offset.
         */
        void translate(const glm::vec3& offset);

        /**
         * @brief Resets the transform to identity.
         */
        void reset();

        /**
         * @brief Builds the local transform matrix.
         *
         * @return Local transform matrix.
         */
        [[nodiscard]] glm::mat4 matrix() const;

    private:
        glm::vec3 position_{ 0.0f, 0.0f, 0.0f };
        glm::quat rotation_{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 scale_{ 1.0f, 1.0f, 1.0f };
    };

}