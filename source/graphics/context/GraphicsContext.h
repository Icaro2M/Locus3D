#pragma once

#include "graphics/common/GraphicsConfig.h"
#include "graphics/common/GraphicsResult.h"
#include "graphics/context/GraphicsCapabilities.h"

namespace locus::graphics
{

    class Window;

    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        GraphicsContext(const GraphicsContext&) = delete;
        GraphicsContext& operator=(const GraphicsContext&) = delete;

        GraphicsContext(GraphicsContext&&) = delete;
        GraphicsContext& operator=(GraphicsContext&&) = delete;

        [[nodiscard]] virtual GraphicsResult<void> initialize(
            Window& window,
            const GraphicsConfig& config) = 0;

        virtual void shutdown() = 0;

        virtual void make_current() = 0;
        virtual void clear_current() = 0;

        virtual void swap_buffers() = 0;
        virtual void set_vsync(bool enabled) = 0;

        [[nodiscard]] virtual const GraphicsCapabilities& capabilities() const = 0;

    protected:
        GraphicsContext() = default;
    };

}