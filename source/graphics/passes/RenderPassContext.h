/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/camera/Camera.h"
#include "graphics/renderer/RenderPipeline.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderScene.h"
#include "graphics/viewport/Viewport.h"

namespace locus::graphics
{
    /**
     * @brief Shared state passed to render passes during execution.
     *
     * RenderPassContext keeps pass implementations decoupled from the owning
     * render loop while still exposing the active renderer, scene, viewport,
     * camera, and pipeline objects.
     *
     * @note Pointers are non-owning and may be null when a pass does not require
     * a specific subsystem.
     */
    struct RenderPassContext
    {
        Renderer* renderer = nullptr;
        RenderPipeline* pipeline = nullptr;
        RenderScene* scene = nullptr;
        Viewport* viewport = nullptr;
        Camera* camera = nullptr;
    };
}
