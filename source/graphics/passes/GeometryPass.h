/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/passes/IRenderPass.h"
#include "graphics/renderer/RenderQueue.h"

namespace locus::graphics
{
    /**
     * @brief Renders default-layer scene geometry.
     *
     * GeometryPass extracts objects from the scene default layer, builds a local
     * render queue, sorts it, and submits it to the active renderer.
     */
    class GeometryPass final : public IRenderPass
    {
    public:
        GeometryPass() = default;
        ~GeometryPass() override = default;

        GeometryPass(const GeometryPass&) = delete;
        GeometryPass& operator=(const GeometryPass&) = delete;

        GeometryPass(GeometryPass&&) noexcept = default;
        GeometryPass& operator=(GeometryPass&&) noexcept = default;

        /**
         * @brief Returns the pass name.
         *
         * @return "GeometryPass".
         */
        [[nodiscard]] const char* name() const override;

        /**
         * @brief Renders the default scene layer using the context renderer.
         *
         * @param context Render pass context containing renderer and scene.
         * @return Empty result on success, or a graphics error on failure.
         */
        GraphicsResult<void> execute(RenderPassContext& context) override;

    private:
        RenderQueue queue_;
    };
}
