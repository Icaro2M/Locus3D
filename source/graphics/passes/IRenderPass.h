#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/passes/RenderPassContext.h"

namespace locus::graphics
{
    class IRenderPass
    {
    public:
        IRenderPass() = default;
        virtual ~IRenderPass() = default;

        IRenderPass(const IRenderPass&) = delete;
        IRenderPass& operator=(const IRenderPass&) = delete;

        IRenderPass(IRenderPass&&) noexcept = default;
        IRenderPass& operator=(IRenderPass&&) noexcept = default;

        [[nodiscard]] virtual const char* name() const = 0;

        virtual GraphicsResult<void> execute(RenderPassContext& context) = 0;
    };
}