/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/picking/PickingId.h"
#include "graphics/scene/RenderObject.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief Result returned by a viewport picking query.
     */
    struct PickingResult
    {
        /**
         * @brief True when the query selected an object.
         */
        bool hit = false;

        /**
         * @brief Encoded picking ID read from the picking buffer.
         */
        PickingId pickingId = PickingId::invalid();

        /**
         * @brief Scene object ID associated with the hit.
         */
        RenderObject::Id objectId = 0;

        /**
         * @brief Depth value sampled at the hit pixel.
         */
        float depth = 1.0f;

        /**
         * @brief Optional world-space hit position.
         */
        glm::vec3 worldPosition{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Optional world-space hit normal.
         */
        glm::vec3 worldNormal{ 0.0f, 1.0f, 0.0f };

        /**
         * @brief Creates a miss result.
         *
         * @return Picking result with hit set to false.
         */
        [[nodiscard]] static PickingResult miss()
        {
            return PickingResult{};
        }

        /**
         * @brief Creates an object-hit result.
         *
         * @param pickingId Encoded picking ID.
         * @param objectId Scene object ID.
         * @param depth Depth value at the hit pixel.
         * @return Picking result with hit set to true.
         */
        [[nodiscard]] static PickingResult object_hit(
            PickingId pickingId,
            RenderObject::Id objectId,
            float depth
        )
        {
            PickingResult result;
            result.hit = true;
            result.pickingId = pickingId;
            result.objectId = objectId;
            result.depth = depth;
            return result;
        }
    };
}
