/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

namespace locus::application {

    /**
     * @brief Describes global product options for the application runtime.
     */
    struct ApplicationConfig {
        /**
         * @brief Initial title of the main application window.
         */
        std::string title = "Locus3D";

        /**
         * @brief Initial logical width of the main window.
         */
        std::int32_t initialWidth = 1280;

        /**
         * @brief Initial logical height of the main window.
         */
        std::int32_t initialHeight = 720;

        /**
         * @brief True when the main window should start maximized.
         */
        bool startMaximized = false;

        /**
         * @brief True when the operating-system window frame is requested.
         */
        bool decorated = true;

        /**
         * @brief True when vertical synchronization is requested.
         */
        bool enableVSync = true;

        /**
         * @brief Maximum frame delta delivered to application consumers.
         */
        double maximumFrameDeltaSeconds = 0.25;
    };

} // namespace locus::application
