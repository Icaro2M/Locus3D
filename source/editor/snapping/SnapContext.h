/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "editor/transform/TransformSpace.h"

#include <glm/glm.hpp>

namespace locus::editor {

    class EditorScene;

    /**
     * @brief Input data used to evaluate a snapping operation.
     */
    struct SnapContext {
        /**
         * @brief Optional scene used by providers that inspect editor objects.
         */
        const EditorScene* scene = nullptr;

        /**
         * @brief Position at the start of the interaction.
         */
        glm::vec3 originalPosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Current candidate position before snapping.
         */
        glm::vec3 candidatePosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Reference origin used by increment and angle snapping.
         */
        glm::vec3 referenceOrigin{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Optional view direction used by screen-aware providers.
         */
        glm::vec3 viewDirection{ 0.0f, 0.0f, -1.0f };

        /**
         * @brief Optional active node affected by the snapping operation.
         */
        SceneNodeId activeNode{};

        /**
         * @brief Transform space used by the current interaction.
         */
        TransformSpace space = TransformSpace::World;

        /**
         * @brief Optional maximum distance override.
         *
         * A negative value means SnapSettings::max_distance should be used.
         */
        float maxDistanceOverride = -1.0f;

        /**
         * @brief Returns the maximum distance to use for this context.
         *
         * @param fallback Fallback distance from SnapSettings.
         * @return Effective maximum distance.
         */
        [[nodiscard]] float effective_max_distance(float fallback) const
        {
            return maxDistanceOverride >= 0.0f ? maxDistanceOverride : fallback;
        }
    };

} // namespace locus::editor