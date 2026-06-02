/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsConfig.h"
#include "graphics/common/GraphicsResult.h"
#include "graphics/context/GraphicsCapabilities.h"
#include "graphics/context/GraphicsContext.h"

namespace locus::graphics
{
    class Window;

    /**
     * @brief OpenGL implementation of the graphics context interface.
     */
    class OpenGLContext final : public GraphicsContext
    {
    public:
        /**
         * @brief Creates an uninitialized OpenGL context wrapper.
         */
        OpenGLContext() = default;

        /**
         * @brief Releases the OpenGL context wrapper.
         */
        ~OpenGLContext() override;

        OpenGLContext(const OpenGLContext&) = delete;
        OpenGLContext& operator=(const OpenGLContext&) = delete;

        OpenGLContext(OpenGLContext&&) = delete;
        OpenGLContext& operator=(OpenGLContext&&) = delete;

        /**
         * @brief Initializes OpenGL for the given window.
         *
         * @param window Window that owns the native OpenGL context.
         * @param config Requested graphics configuration.
         * @return Success or initialization error.
         */
        [[nodiscard]] GraphicsResult<void> initialize(
            Window& window,
            const GraphicsConfig& config) override;

        /**
         * @brief Clears the current context and marks this wrapper uninitialized.
         */
        void shutdown() override;

        /**
         * @brief Makes the wrapped OpenGL context current.
         */
        void make_current() override;

        /**
         * @brief Clears the current OpenGL context.
         */
        void clear_current() override;

        /**
         * @brief Presents the current back buffer.
         */
        void swap_buffers() override;

        /**
         * @brief Enables or disables OpenGL swap interval synchronization.
         *
         * @param enabled True to enable vsync.
         */
        void set_vsync(bool enabled) override;

        /**
         * @brief Returns capabilities read from the active OpenGL context.
         *
         * @return Read-only graphics capabilities.
         */
        [[nodiscard]] const GraphicsCapabilities& capabilities() const override;

    private:
        /**
         * @brief Loads OpenGL function pointers through GLAD.
         *
         * @return Success or function loading error.
         */
        [[nodiscard]] GraphicsResult<void> load_opengl_functions();

        /**
         * @brief Validates that the current OpenGL version satisfies the request.
         *
         * @param config Requested graphics configuration.
         * @return Success or version mismatch error.
         */
        [[nodiscard]] GraphicsResult<void> validate_version(const GraphicsConfig& config) const;

        /**
         * @brief Reads backend capability values from OpenGL.
         */
        void read_capabilities();

        /**
         * @brief Configures OpenGL debug output when requested and supported.
         *
         * @param enabled True to enable debug output.
         */
        void configure_debug_output(bool enabled);

    private:
        Window* window_ = nullptr;
        GraphicsCapabilities capabilities_{};
        bool initialized_ = false;
    };

}
