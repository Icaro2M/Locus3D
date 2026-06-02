#pragma once

#include "graphics/common/GraphicsConfig.h"
#include "graphics/common/GraphicsResult.h"
#include "graphics/context/GraphicsCapabilities.h"
#include "graphics/context/GraphicsContext.h"

namespace locus::graphics
{

    class Window;

    class OpenGLContext final : public GraphicsContext
    {
    public:
        OpenGLContext() = default;
        ~OpenGLContext() override;

        OpenGLContext(const OpenGLContext&) = delete;
        OpenGLContext& operator=(const OpenGLContext&) = delete;

        OpenGLContext(OpenGLContext&&) = delete;
        OpenGLContext& operator=(OpenGLContext&&) = delete;

        [[nodiscard]] GraphicsResult<void> initialize(
            Window& window,
            const GraphicsConfig& config) override;

        void shutdown() override;

        void make_current() override;
        void clear_current() override;

        void swap_buffers() override;
        void set_vsync(bool enabled) override;

        [[nodiscard]] const GraphicsCapabilities& capabilities() const override;

    private:
        [[nodiscard]] GraphicsResult<void> load_opengl_functions();
        [[nodiscard]] GraphicsResult<void> validate_version(const GraphicsConfig& config) const;

        void read_capabilities();
        void configure_debug_output(bool enabled);

    private:
        Window* window_ = nullptr;
        GraphicsCapabilities capabilities_{};
        bool initialized_ = false;
    };

}