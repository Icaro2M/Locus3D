/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/ApplicationConfig.h"
#include "application/ApplicationResult.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/window/Window.h"

#include <cstdint>

namespace locus::application {

    /**
     * @brief Owns the graphics window and context used by the application.
     *
     * This class translates product configuration into graphics configuration
     * and coordinates lifecycle operations. Native window behavior remains in
     * graphics::Window and graphics::OpenGLContext.
     */
    class ApplicationWindow {
    public:
        /**
         * @brief Creates an uninitialized application window.
         */
        ApplicationWindow() = default;

        /**
         * @brief Shuts down the owned graphics resources.
         */
        ~ApplicationWindow();

        ApplicationWindow(const ApplicationWindow&) = delete;
        ApplicationWindow& operator=(const ApplicationWindow&) = delete;
        ApplicationWindow(ApplicationWindow&&) = delete;
        ApplicationWindow& operator=(ApplicationWindow&&) = delete;

        /**
         * @brief Creates the graphics window and initializes its OpenGL context.
         *
         * @param config Product-level application configuration.
         * @return Success or an application initialization error.
         */
        [[nodiscard]] ApplicationResult<void> initialize(
            const ApplicationConfig& config);

        /**
         * @brief Releases the graphics context and native window.
         *
         * Calling this method on an uninitialized window is safe.
         */
        void shutdown();

        /**
         * @brief Checks whether the graphics window is ready for use.
         *
         * @return True after successful initialization and before shutdown.
         */
        [[nodiscard]] bool initialized() const noexcept;

        /**
         * @brief Polls pending platform events.
         */
        void process_events() const;

        /**
         * @brief Presents the current back buffer.
         */
        void present();

        /**
         * @brief Checks whether application window closure was requested.
         *
         * @return True when uninitialized or when the native window should close.
         */
        [[nodiscard]] bool should_close() const;

        /**
         * @brief Requests closure of the native window.
         */
        void request_close();

        /**
         * @brief Returns the current logical width.
         *
         * @return Logical width, or zero while uninitialized.
         */
        [[nodiscard]] std::int32_t width() const noexcept;

        /**
         * @brief Returns the current logical height.
         *
         * @return Logical height, or zero while uninitialized.
         */
        [[nodiscard]] std::int32_t height() const noexcept;

        /**
         * @brief Returns the current framebuffer width.
         *
         * @return Framebuffer width, or zero while uninitialized.
         */
        [[nodiscard]] std::int32_t framebuffer_width() const noexcept;

        /**
         * @brief Returns the current framebuffer height.
         *
         * @return Framebuffer height, or zero while uninitialized.
         */
        [[nodiscard]] std::int32_t framebuffer_height() const noexcept;

        /**
         * @brief Returns the product configuration used during initialization.
         *
         * @return Read-only configuration snapshot.
         */
        [[nodiscard]] const ApplicationConfig& configuration() const noexcept;

    private:
        ApplicationConfig configuration_{};
        graphics::Window window_{};
        graphics::OpenGLContext context_{};
        bool initialized_ = false;
    };

} // namespace locus::application
