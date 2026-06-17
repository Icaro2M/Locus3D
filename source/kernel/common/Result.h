/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Error.h"

#include <utility>
#include <variant>

namespace locus::kernel {

/**
 * @brief Stores either a successful value or an Error.
 *
 * @tparam T Value type returned on success.
 */
template <typename T>
class Result {
public:
    /**
     * @brief Creates a successful result by copying a value.
     *
     * @param value Value to store.
     */
    Result(const T& value)
        : storage_(value)
    {
    }

    /**
     * @brief Creates a successful result by moving a value.
     *
     * @param value Value to store.
     */
    Result(T&& value)
        : storage_(std::move(value))
    {
    }

    /**
     * @brief Creates a failed result from an Error.
     *
     * @param error Error to store.
     */
    Result(Error error)
        : storage_(std::move(error))
    {
    }

    /**
     * @brief Checks whether the result contains a value.
     *
     * @return True when the result is successful.
     */
    [[nodiscard]] bool is_ok() const
    {
        return std::holds_alternative<T>(storage_);
    }

    /**
     * @brief Checks whether the result contains an error.
     *
     * @return True when the result failed.
     */
    [[nodiscard]] bool is_error() const
    {
        return !is_ok();
    }

    /**
     * @brief Converts the result to a boolean success test.
     *
     * @return True when the result contains a value.
     */
    [[nodiscard]] explicit operator bool() const
    {
        return is_ok();
    }

    /**
     * @brief Returns mutable access to the stored value.
     *
     * @return Mutable value reference.
     * @note Calling this on an error result throws std::bad_variant_access.
     */
    [[nodiscard]] T& value()
    {
        return std::get<T>(storage_);
    }

    /**
     * @brief Returns read-only access to the stored value.
     *
     * @return Read-only value reference.
     * @note Calling this on an error result throws std::bad_variant_access.
     */
    [[nodiscard]] const T& value() const
    {
        return std::get<T>(storage_);
    }

    /**
     * @brief Returns mutable access to the stored error.
     *
     * @return Mutable error reference.
     * @note Calling this on a successful result throws std::bad_variant_access.
     */
    [[nodiscard]] Error& error()
    {
        return std::get<Error>(storage_);
    }

    /**
     * @brief Returns read-only access to the stored error.
     *
     * @return Read-only error reference.
     * @note Calling this on a successful result throws std::bad_variant_access.
     */
    [[nodiscard]] const Error& error() const
    {
        return std::get<Error>(storage_);
    }

    /**
     * @brief Returns the stored value or a fallback when the result failed.
     *
     * @param fallback Value returned when this result contains an error.
     * @return Stored value or fallback.
     */
    [[nodiscard]] T value_or(T fallback) const
    {
        if (is_ok()) {
            return value();
        }

        return fallback;
    }

    /**
     * @brief Creates a successful result.
     *
     * @param value Value to store.
     * @return Successful result.
     */
    [[nodiscard]] static Result ok(T value)
    {
        return Result(std::move(value));
    }

    /**
     * @brief Creates a failed result from an Error.
     *
     * @param error Error to store.
     * @return Failed result.
     */
    [[nodiscard]] static Result fail(Error error)
    {
        return Result(std::move(error));
    }

    /**
     * @brief Creates a failed result from an error code and optional message.
     *
     * @param code Error category.
     * @param message Human-readable diagnostic message.
     * @return Failed result.
     */
    [[nodiscard]] static Result fail(ErrorCode code, std::string message = {})
    {
        return Result(Error::make(code, std::move(message)));
    }

private:
    std::variant<T, Error> storage_;
};

/**
 * @brief Specialized Result for operations that only report success or failure.
 */
template <>
class Result<void> {
public:
    /**
     * @brief Creates a successful void result.
     */
    Result()
        : error_(Error::none())
    {
    }

    /**
     * @brief Creates a failed void result.
     *
     * @param error Error to store.
     */
    Result(Error error)
        : error_(std::move(error))
    {
    }

    /**
     * @brief Checks whether the result represents success.
     *
     * @return True when the stored error is ErrorCode::None.
     */
    [[nodiscard]] bool is_ok() const
    {
        return error_.is_ok();
    }

    /**
     * @brief Checks whether the result represents failure.
     *
     * @return True when the stored error is not ErrorCode::None.
     */
    [[nodiscard]] bool is_error() const
    {
        return error_.is_error();
    }

    /**
     * @brief Converts the result to a boolean success test.
     *
     * @return True when the result represents success.
     */
    [[nodiscard]] explicit operator bool() const
    {
        return is_ok();
    }

    /**
     * @brief Returns mutable access to the stored error.
     *
     * @return Mutable error reference.
     */
    [[nodiscard]] Error& error()
    {
        return error_;
    }

    /**
     * @brief Returns read-only access to the stored error.
     *
     * @return Read-only error reference.
     */
    [[nodiscard]] const Error& error() const
    {
        return error_;
    }

    /**
     * @brief Creates a successful void result.
     *
     * @return Successful result.
     */
    [[nodiscard]] static Result ok()
    {
        return Result();
    }

    /**
     * @brief Creates a failed void result from an Error.
     *
     * @param error Error to store.
     * @return Failed result.
     */
    [[nodiscard]] static Result fail(Error error)
    {
        return Result(std::move(error));
    }

    /**
     * @brief Creates a failed void result from an error code and optional message.
     *
     * @param code Error category.
     * @param message Human-readable diagnostic message.
     * @return Failed result.
     */
    [[nodiscard]] static Result fail(ErrorCode code, std::string message = {})
    {
        return Result(Error::make(code, std::move(message)));
    }

private:
    Error error_;
};

}
