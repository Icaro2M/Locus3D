/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <limits>

namespace locus::editor {

    /**
     * @brief Kind of element produced by a snap provider.
     */
    enum class SnapTargetType {
        /**
         * @brief No target was found.
         */
        None,

        /**
         * @brief Target is a grid point.
         */
        GridPoint,

        /**
         * @brief Target is a mesh vertex.
         */
        Vertex,

        /**
         * @brief Target is a mesh edge.
         */
        Edge,

        /**
         * @brief Target is a mesh face.
         */
        Face,

        /**
         * @brief Target is a linear increment position.
         */
        Increment,

        /**
         * @brief Target is an angular increment position.
         */
        Angle
    };

    /**
     * @brief Describes the geometric element selected by snapping.
     */
    struct SnapTarget {
        /**
         * @brief Target element type.
         */
        SnapTargetType type = SnapTargetType::None;

        /**
         * @brief World-space target position.
         */
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Optional world-space target normal.
         */
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };

        /**
         * @brief Optional editor scene node that owns the snapped element.
         */
        SceneNodeId node{};

        /**
         * @brief Optional component index for providers that expose mesh elements.
         */
        std::uint64_t component = std::numeric_limits<std::uint64_t>::max();

        /**
         * @brief Checks whether this target references a usable snap element.
         *
         * @return True when the target type is not None.
         */
        [[nodiscard]] bool is_valid() const
        {
            return type != SnapTargetType::None;
        }
    };

} // namespace locus::editor