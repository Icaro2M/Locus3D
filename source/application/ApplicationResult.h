/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/ApplicationError.h"

#include <utility>

namespace locus::application {

    /**
     * @brief Stores either a successful value or an application error.
     *
     * @tparam T Successful value type.
     */
    template <typename T>
    class ApplicationResult {
    public:
        /**
         * @brief Creates a successful result from a copied value.
         *
         * @param value Successful result value.
         */
        ApplicationResult(const T& value)
            : value_(value)
            , error_(ApplicationError::none())
            , hasValue_(true)
        {
        }

        /**
         * @brief Creates a successful result from a moved value.
         *
         * @param value Successful result value.
         */
        ApplicationResult(T&& value)
            : value_(std::move(value))
            , error_(ApplicationError::none())
            , hasValue_(true)
        {
        }

        /**
         * @brief Creates a failed result from an application error.
         *
         * @param error Error value.
         */
        ApplicationResult(ApplicationError error)
            : error_(std::move(error))
        {
        }

        /**
         * @brief Checks whether the result contains a value.
         *
         * @return True when the operation succeeded.
         */
        [[nodiscard]] bool ok() const noexcept
        {
            return hasValue_;
        }

        /**
         * @brief Checks whether the operation failed.
         *
         * @return True when the result contains an error.
         */
        [[nodiscard]] bool failed() const noexcept
        {
            return !ok();
        }

        /**
         * @brief Converts the result to a success flag.
         *
         * @return True when the operation succeeded.
         */
        [[nodiscard]] explicit operator bool() const noexcept
        {
            return ok();
        }

        /**
         * @brief Returns the stored value.
         *
         * @return Mutable successful value.
         * @pre ok() is true.
         */
        [[nodiscard]] T& value() noexcept
        {
            return value_;
        }

        /**
         * @brief Returns the stored value.
         *
         * @return Read-only successful value.
         * @pre ok() is true.
         */
        [[nodiscard]] const T& value() const noexcept
        {
            return value_;
        }

        /**
         * @brief Moves the successful value out of the result.
         *
         * @return Moved successful value.
         * @pre ok() is true.
         */
        [[nodiscard]] T&& move_value() noexcept
        {
            return std::move(value_);
        }

        /**
         * @brief Returns the stored error.
         *
         * @return Error value, or an empty error after success.
         */
        [[nodiscard]] const ApplicationError& error() const noexcept
        {
            return error_;
        }

    private:
        T value_{};
        ApplicationError error_{};
        bool hasValue_ = false;
    };

    /**
     * @brief Stores success or failure for operations that return no value.
     */
    template <>
    class ApplicationResult<void> {
    public:
        /**
         * @brief Creates a successful result.
         */
        ApplicationResult() = default;

        /**
         * @brief Creates a failed result from an application error.
         *
         * @param error Error value.
         */
        ApplicationResult(ApplicationError error)
            : error_(std::move(error))
            , success_(false)
        {
        }

        /**
         * @brief Checks whether the operation succeeded.
         *
         * @return True when no error was reported.
         */
        [[nodiscard]] bool ok() const noexcept
        {
            return success_;
        }

        /**
         * @brief Checks whether the operation failed.
         *
         * @return True when the result contains an error.
         */
        [[nodiscard]] bool failed() const noexcept
        {
            return !ok();
        }

        /**
         * @brief Converts the result to a success flag.
         *
         * @return True when no error was reported.
         */
        [[nodiscard]] explicit operator bool() const noexcept
        {
            return ok();
        }

        /**
         * @brief Returns the stored error.
         *
         * @return Error value, or an empty error after success.
         */
        [[nodiscard]] const ApplicationError& error() const noexcept
        {
            return error_;
        }

    private:
        ApplicationError error_{};
        bool success_ = true;
    };

} // namespace locus::application
