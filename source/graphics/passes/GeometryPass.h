#pragma once

#include "graphics/passes/IRenderPass.h"
#include "graphics/renderer/RenderQueue.h"

namespace locus::graphics
{
    class GeometryPass final : public IRenderPass
    {
    public:
        GeometryPass() = default;
        ~GeometryPass() override = default;

        GeometryPass(const GeometryPass&) = delete;
        GeometryPass& operator=(const GeometryPass&) = delete;

        GeometryPass(GeometryPass&&) noexcept = default;
        GeometryPass& operator=(GeometryPass&&) noexcept = default;

        [[nodiscard]] const char* name() const override;

        GraphicsResult<void> execute(RenderPassContext& context) override;

    private:
        RenderQueue queue_;
    };
}