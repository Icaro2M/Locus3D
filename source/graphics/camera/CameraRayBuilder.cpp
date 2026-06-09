#include "graphics/camera/CameraRayBuilder.h"

#include <algorithm>
#include <glm/gtc/matrix_inverse.hpp>

namespace locus::graphics
{
    CameraRay CameraRayBuilder::from_viewport_pixel(
        const Camera& camera,
        const ViewportRect& viewport,
        float pixelX,
        float pixelY
    )
    {
        const glm::vec2 ndc = pixel_to_ndc(viewport, pixelX, pixelY);

        const glm::mat4 inverseViewProjection = glm::inverse(camera.view_projection_matrix());

        const glm::vec4 nearClip{ ndc.x, ndc.y, -1.0f, 1.0f };
        const glm::vec4 farClip{ ndc.x, ndc.y, 1.0f, 1.0f };

        glm::vec4 nearWorld = inverseViewProjection * nearClip;
        glm::vec4 farWorld = inverseViewProjection * farClip;

        if (nearWorld.w != 0.0f)
        {
            nearWorld /= nearWorld.w;
        }

        if (farWorld.w != 0.0f)
        {
            farWorld /= farWorld.w;
        }

        CameraRay ray;
        ray.origin = glm::vec3{ nearWorld };
        ray.direction = glm::normalize(glm::vec3{ farWorld - nearWorld });

        return ray;
    }

    glm::vec2 CameraRayBuilder::pixel_to_ndc(
        const ViewportRect& viewport,
        float pixelX,
        float pixelY
    )
    {
        const float width = std::max(static_cast<float>(viewport.width), 1.0f);
        const float height = std::max(static_cast<float>(viewport.height), 1.0f);

        const float localX = pixelX - static_cast<float>(viewport.x);
        const float localY = pixelY - static_cast<float>(viewport.y);

        const float ndcX = (localX / width) * 2.0f - 1.0f;
        const float ndcY = 1.0f - (localY / height) * 2.0f;

        return glm::vec2{ ndcX, ndcY };
    }
}