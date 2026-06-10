#pragma once

#include "graphics/picking/PickingId.h"
#include "graphics/scene/RenderObject.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    struct PickingResult
    {
        bool hit = false;

        PickingId pickingId = PickingId::invalid();
        RenderObject::Id objectId = 0;

        float depth = 1.0f;

        glm::vec3 worldPosition{ 0.0f, 0.0f, 0.0f };
        glm::vec3 worldNormal{ 0.0f, 1.0f, 0.0f };

        [[nodiscard]] static PickingResult miss()
        {
            return PickingResult{};
        }

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