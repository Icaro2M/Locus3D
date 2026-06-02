/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "GraphicsError.h"

#include <utility>

namespace locus::graphics
{
    /**
     * @brief Stores either a successful value or a graphics error.
     *
     * @tparam T Successful value type.
     */
    template <typename T>
    class GraphicsResult
    {
    public:
        /**
         * @brief Creates a successful result from a copied value.
         *
         * @param value Successful result value.
         */
        GraphicsResult(const T& value)
            : value_(value), error_(GraphicsError::none()), has_value_(true)
        {
        }

        /**
         * @brief Creates a successful result from a moved value.
         *
         * @param value Successful result value.
         */
        GraphicsResult(T&& value)
            : value_(std::move(value)), error_(GraphicsError::none()), has_value_(true)
        {
        }

        /**
         * @brief Creates a failed result from a graphics error.
         *
         * @param error Error value.
         */
        GraphicsResult(GraphicsError error)
            : value_(), error_(std::move(error)), has_value_(false)
        {
        }

        /**
         * @brief Checks whether the result contains a value.
         *
         * @return True when the operation succeeded.
         */
        [[nodiscard]] bool ok() const
        {
            return has_value_;
        }

        /**
         * @brief Converts the result to a success flag.
         *
         * @return True when the operation succeeded.
         */
        [[nodiscard]] explicit operator bool() const
        {
            return ok();
        }

        /**
         * @brief Returns the stored value.
         *
         * @return Read-only successful value.
         */
        [[nodiscard]] const T& value() const
        {
            return value_;
        }

        /**
         * @brief Returns the stored value.
         *
         * @return Mutable successful value.
         */
        [[nodiscard]] T& value()
        {
            return value_;
        }

        /**
         * @brief Moves the stored successful value out of the result.
         *
         * @return Moved successful value.
         */
        [[nodiscard]] T&& move_value()
        {
            return std::move(value_);
        }

        /**
         * @brief Returns the stored error.
         *
         * @return Read-only error value.
         */
        [[nodiscard]] const GraphicsError& error() const
        {
            return error_;
        }

    private:
        T value_{};
        GraphicsError error_{};
        bool has_value_ = false;
    };

    /**
     * @brief Stores success or failure for operations that return no value.
     */
    template <>
    class GraphicsResult<void>
    {
    public:
        /**
         * @brief Creates a successful void result.
         */
        GraphicsResult()
            : error_(GraphicsError::none()), success_(true)
        {
        }

        /**
         * @brief Creates a failed void result from a graphics error.
         *
         * @param error Error value.
         */
        GraphicsResult(GraphicsError error)
            : error_(std::move(error)), success_(false)
        {
        }

        /**
         * @brief Checks whether the operation succeeded.
         *
         * @return True when no error was reported.
         */
        [[nodiscard]] bool ok() const
        {
            return success_;
        }

        /**
         * @brief Converts the result to a success flag.
         *
         * @return True when no error was reported.
         */
        [[nodiscard]] explicit operator bool() const
        {
            return ok();
        }

        /**
         * @brief Returns the stored error.
         *
         * @return Read-only error value.
         */
        [[nodiscard]] const GraphicsError& error() const
        {
            return error_;
        }

    private:
        GraphicsError error_{};
        bool success_ = true;
    };

}
