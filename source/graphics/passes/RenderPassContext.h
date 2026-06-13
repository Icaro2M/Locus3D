#pragma once

#include "graphics/camera/Camera.h"
#include "graphics/renderer/RenderPipeline.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderScene.h"
#include "graphics/viewport/Viewport.h"

namespace locus::graphics
{
    struct RenderPassContext
    {
        Renderer* renderer = nullptr;
        RenderPipeline* pipeline = nullptr;
        RenderScene* scene = nullptr;
        Viewport* viewport = nullptr;
        Camera* camera = nullptr;
    };
}