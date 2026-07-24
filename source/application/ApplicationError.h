/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <utility>

namespace locus::application {

    /**
     * @brief Enumerates fundamental application-layer error categories.
     */
    enum class ApplicationErrorCode {
        /**
         * @brief No error was reported.
         */
        None,

        /**
         * @brief The application state does not allow the requested operation.
         */
        InvalidState,

        /**
         * @brief The application configuration is invalid.
         */
        InvalidConfiguration,

        /**
         * @brief Application initialization could not be completed.
         */
        InitializationFailed,

        /**
         * @brief The application failed while running.
         */
        RuntimeFailure,

        /**
         * @brief An internal application invariant was violated.
         */
        InternalFailure
    };

    /**
     * @brief Stores an application error code and diagnostic message.
     */
    struct ApplicationError {
        /**
         * @brief Machine-readable error category.
         */
        ApplicationErrorCode code = ApplicationErrorCode::None;

        /**
         * @brief Human-readable diagnostic message.
         */
        std::string message{};

        /**
         * @brief Checks whether this value represents an error.
         *
         * @return True when the error code is not None.
         */
        [[nodiscard]] bool has_error() const noexcept
        {
            return code != ApplicationErrorCode::None;
        }

        /**
         * @brief Converts the error to a boolean failure test.
         *
         * @return True when this value represents an error.
         */
        [[nodiscard]] explicit operator bool() const noexcept
        {
            return has_error();
        }

        /**
         * @brief Creates an empty error value.
         *
         * @return Error value with code None.
         */
        [[nodiscard]] static ApplicationError none()
        {
            return {};
        }

        /**
         * @brief Creates an application error.
         *
         * @param code Error category.
         * @param message Human-readable diagnostic message.
         * @return Populated error value.
         */
        [[nodiscard]] static ApplicationError make(
            ApplicationErrorCode code,
            std::string message = {})
        {
            return ApplicationError{ code, std::move(message) };
        }
    };

} // namespace locus::application
