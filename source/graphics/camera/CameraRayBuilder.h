#pragma once

#include "graphics/camera/Camera.h"
#include "graphics/common/GraphicsTypes.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    struct CameraRay
    {
        glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
        glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
    };

    class CameraRayBuilder
    {
    public:
        CameraRayBuilder() = delete;

        [[nodiscard]] static CameraRay from_viewport_pixel(
            const Camera& camera,
            const ViewportRect& viewport,
            float pixelX,
            float pixelY
        );

    private:
        [[nodiscard]] static glm::vec2 pixel_to_ndc(
            const ViewportRect& viewport,
            float pixelX,
            float pixelY
        );
    };
}