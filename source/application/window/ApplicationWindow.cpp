/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/window/ApplicationWindow.h"

#include "graphics/common/GraphicsConfig.h"

#include <cmath>
#include <string>

namespace locus::application {

    namespace {

        [[nodiscard]] ApplicationResult<void> invalid_configuration(
            std::string message)
        {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidConfiguration,
                std::move(message));
        }

        [[nodiscard]] ApplicationResult<void> initialization_failure(
            const char* operation,
            const graphics::GraphicsError& error)
        {
            std::string message = operation;

            if (!error.message.empty()) {
                message += ": ";
                message += error.message;
            }

            return ApplicationError::make(
                ApplicationErrorCode::InitializationFailed,
                std::move(message));
        }

    } // namespace

    ApplicationWindow::~ApplicationWindow()
    {
        shutdown();
    }

    ApplicationResult<void> ApplicationWindow::initialize(
        const ApplicationConfig& config)
    {
        if (initialized_) {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidState,
                "ApplicationWindow is already initialized.");
        }

        if (config.title.empty()) {
            return invalid_configuration(
                "Application window title cannot be empty.");
        }

        if (config.initialWidth <= 0 || config.initialHeight <= 0) {
            return invalid_configuration(
                "Application window dimensions must be positive.");
        }

        if (!std::isfinite(config.maximumFrameDeltaSeconds)
            || config.maximumFrameDeltaSeconds < 0.0) {
            return invalid_configuration(
                "Maximum frame delta must be finite and non-negative.");
        }

        graphics::WindowCreateInfo windowInfo{};
        windowInfo.width = config.initialWidth;
        windowInfo.height = config.initialHeight;
        windowInfo.title = config.title;
        windowInfo.decorated = config.decorated;
        windowInfo.maximized = config.startMaximized;

        const graphics::GraphicsResult<void> windowResult =
            window_.create(windowInfo);

        if (!windowResult) {
            return initialization_failure(
                "Failed to create the application window",
                windowResult.error());
        }

        graphics::GraphicsConfig graphicsConfig{};
        graphicsConfig.enableVSync = config.enableVSync;

        const graphics::GraphicsResult<void> contextResult =
            context_.initialize(window_, graphicsConfig);

        if (!contextResult) {
            window_.destroy();
            return initialization_failure(
                "Failed to initialize the application graphics context",
                contextResult.error());
        }

        configuration_ = config;
        initialized_ = true;
        return {};
    }

    void ApplicationWindow::shutdown()
    {
        if (!initialized_) {
            return;
        }

        context_.shutdown();
        window_.destroy();
        initialized_ = false;
    }

    bool ApplicationWindow::initialized() const noexcept
    {
        return initialized_;
    }

    void ApplicationWindow::process_events() const
    {
        if (initialized_) {
            window_.poll_events();
        }
    }

    void ApplicationWindow::present()
    {
        if (initialized_) {
            context_.swap_buffers();
        }
    }

    bool ApplicationWindow::should_close() const
    {
        return !initialized_ || window_.should_close();
    }

    void ApplicationWindow::request_close()
    {
        if (initialized_) {
            window_.request_close();
        }
    }

    std::int32_t ApplicationWindow::width() const noexcept
    {
        return window_.width();
    }

    std::int32_t ApplicationWindow::height() const noexcept
    {
        return window_.height();
    }

    std::int32_t ApplicationWindow::framebuffer_width() const noexcept
    {
        return window_.framebuffer_width();
    }

    std::int32_t ApplicationWindow::framebuffer_height() const noexcept
    {
        return window_.framebuffer_height();
    }

    const ApplicationConfig& ApplicationWindow::configuration() const noexcept
    {
        return configuration_;
    }

} // namespace locus::application
