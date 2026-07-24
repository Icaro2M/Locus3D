/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace locus::application {

    /**
     * @brief Enumerates the high-level application runtime phases.
     */
    enum class ApplicationPhase {
        /**
         * @brief Runtime initialization has not started.
         */
        Uninitialized,

        /**
         * @brief Runtime subsystems are being initialized.
         */
        Initializing,

        /**
         * @brief The application is executing its main loop.
         */
        Running,

        /**
         * @brief Frame processing is temporarily suspended.
         */
        Suspended,

        /**
         * @brief Runtime shutdown has been requested.
         */
        Stopping,

        /**
         * @brief Runtime shutdown completed normally.
         */
        Stopped,

        /**
         * @brief Runtime initialization or execution failed.
         */
        Failed
    };

    /**
     * @brief Stores process-level state owned by the application runtime.
     */
    struct ApplicationState {
        /**
         * @brief Current high-level runtime phase.
         */
        ApplicationPhase phase = ApplicationPhase::Uninitialized;

        /**
         * @brief True when an orderly application shutdown was requested.
         */
        bool quitRequested = false;

        /**
         * @brief Process exit code selected by the runtime.
         */
        int exitCode = 0;

        /**
         * @brief Index of the next frame to be processed.
         */
        std::uint64_t frameIndex = 0;
    };

} // namespace locus::application
