/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsConfig.h"
#include "graphics/common/GraphicsResult.h"
#include "graphics/context/GraphicsCapabilities.h"

namespace locus::graphics
{
    class Window;

    /**
     * @brief Abstract interface for a graphics API context.
     */
    class GraphicsContext
    {
    public:
        /**
         * @brief Destroys the graphics context interface.
         */
        virtual ~GraphicsContext() = default;

        GraphicsContext(const GraphicsContext&) = delete;
        GraphicsContext& operator=(const GraphicsContext&) = delete;

        GraphicsContext(GraphicsContext&&) = delete;
        GraphicsContext& operator=(GraphicsContext&&) = delete;

        /**
         * @brief Initializes the context for a window.
         *
         * @param window Window that owns the native graphics surface.
         * @param config Graphics runtime configuration.
         * @return Success or initialization error.
         */
        [[nodiscard]] virtual GraphicsResult<void> initialize(
            Window& window,
            const GraphicsConfig& config) = 0;

        /**
         * @brief Releases context-owned graphics resources.
         */
        virtual void shutdown() = 0;

        /**
         * @brief Makes this context current on the calling thread.
         */
        virtual void make_current() = 0;

        /**
         * @brief Clears the current context from the calling thread.
         */
        virtual void clear_current() = 0;

        /**
         * @brief Presents the back buffer to the window surface.
         */
        virtual void swap_buffers() = 0;

        /**
         * @brief Enables or disables vertical synchronization.
         *
         * @param enabled True to enable vsync.
         */
        virtual void set_vsync(bool enabled) = 0;

        /**
         * @brief Returns capabilities reported by the initialized context.
         *
         * @return Read-only graphics capabilities.
         */
        [[nodiscard]] virtual const GraphicsCapabilities& capabilities() const = 0;

    protected:
        /**
         * @brief Allows construction only through concrete context types.
         */
        GraphicsContext() = default;
    };

}
