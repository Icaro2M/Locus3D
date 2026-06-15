/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/passes/RenderPassContext.h"

namespace locus::graphics
{
    /**
     * @brief Interface for one render pipeline pass.
     *
     * Implementations receive a RenderPassContext and are responsible for
     * validating the subsystems they require before issuing rendering work.
     */
    class IRenderPass
    {
    public:
        IRenderPass() = default;
        virtual ~IRenderPass() = default;

        IRenderPass(const IRenderPass&) = delete;
        IRenderPass& operator=(const IRenderPass&) = delete;

        IRenderPass(IRenderPass&&) noexcept = default;
        IRenderPass& operator=(IRenderPass&&) noexcept = default;

        /**
         * @brief Returns the human-readable pass name.
         *
         * @return Stable pass name used for diagnostics and profiling.
         */
        [[nodiscard]] virtual const char* name() const = 0;

        /**
         * @brief Executes the render pass.
         *
         * @param context Non-owning context for the current render frame.
         * @return Empty result on success, or a graphics error on failure.
         */
        virtual GraphicsResult<void> execute(RenderPassContext& context) = 0;
    };
}
