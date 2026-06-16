#pragma once

#include <string>
#include <utility>

namespace locus::kernel {

enum class ErrorCode {
    None,
    InvalidArgument,
    InvalidState,
    NotFound,
    OutOfRange,
    UnsupportedOperation,
    DegenerateGeometry,
    NonManifoldTopology,
    NumericFailure,
    IoError,
    Unknown
};

struct Error {
    ErrorCode code = ErrorCode::None;
    std::string message{};

    [[nodiscard]] bool is_ok() const
    {
        return code == ErrorCode::None;
    }

    [[nodiscard]] bool is_error() const
    {
        return !is_ok();
    }

    [[nodiscard]] explicit operator bool() const
    {
        return is_error();
    }

    [[nodiscard]] static Error none()
    {
        return {};
    }

    [[nodiscard]] static Error make(ErrorCode code, std::string message = {})
    {
        return Error{ code, std::move(message) };
    }
};

}
