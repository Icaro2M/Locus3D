/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <utility>

namespace locus::kernel {

/**
 * @brief Enumerates common error categories returned by kernel operations.
 */
enum class ErrorCode {
    /**
     * @brief Operation completed successfully.
     */
    None,

    /**
     * @brief Caller provided an invalid argument.
     */
    InvalidArgument,

    /**
     * @brief Object state does not allow the requested operation.
     */
    InvalidState,

    /**
     * @brief Requested element or resource was not found.
     */
    NotFound,

    /**
     * @brief Requested index or value is outside the valid range.
     */
    OutOfRange,

    /**
     * @brief Operation is not supported by the current implementation.
     */
    UnsupportedOperation,

    /**
     * @brief Geometry is degenerate and cannot be processed reliably.
     */
    DegenerateGeometry,

    /**
     * @brief Mesh topology violates manifold assumptions.
     */
    NonManifoldTopology,

    /**
     * @brief Numeric computation failed or became unstable.
     */
    NumericFailure,

    /**
     * @brief Input or output operation failed.
     */
    IoError,

    /**
     * @brief Unclassified error.
     */
    Unknown
};

/**
 * @brief Lightweight error object carrying a code and optional message.
 */
struct Error {
    /**
     * @brief Machine-readable error category.
     */
    ErrorCode code = ErrorCode::None;

    /**
     * @brief Human-readable diagnostic message.
     */
    std::string message{};

    /**
     * @brief Checks whether this object represents success.
     *
     * @return True when the error code is ErrorCode::None.
     */
    [[nodiscard]] bool is_ok() const
    {
        return code == ErrorCode::None;
    }

    /**
     * @brief Checks whether this object represents a failure.
     *
     * @return True when the error code is not ErrorCode::None.
     */
    [[nodiscard]] bool is_error() const
    {
        return !is_ok();
    }

    /**
     * @brief Converts the error to a boolean failure test.
     *
     * @return True when this object represents a failure.
     */
    [[nodiscard]] explicit operator bool() const
    {
        return is_error();
    }

    /**
     * @brief Creates a success error object.
     *
     * @return Error object with ErrorCode::None.
     */
    [[nodiscard]] static Error none()
    {
        return {};
    }

    /**
     * @brief Creates an error object from a code and optional message.
     *
     * @param code Error category.
     * @param message Human-readable diagnostic message.
     * @return Error object containing the given data.
     */
    [[nodiscard]] static Error make(ErrorCode code, std::string message = {})
    {
        return Error{ code, std::move(message) };
    }
};

}
