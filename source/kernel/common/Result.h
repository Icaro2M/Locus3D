#pragma once

#include "kernel/common/Error.h"

#include <utility>
#include <variant>

namespace locus::kernel {

template <typename T>
class Result {
public:
    Result(const T& value)
        : storage_(value)
    {
    }

    Result(T&& value)
        : storage_(std::move(value))
    {
    }

    Result(Error error)
        : storage_(std::move(error))
    {
    }

    [[nodiscard]] bool is_ok() const
    {
        return std::holds_alternative<T>(storage_);
    }

    [[nodiscard]] bool is_error() const
    {
        return !is_ok();
    }

    [[nodiscard]] explicit operator bool() const
    {
        return is_ok();
    }

    [[nodiscard]] T& value()
    {
        return std::get<T>(storage_);
    }

    [[nodiscard]] const T& value() const
    {
        return std::get<T>(storage_);
    }

    [[nodiscard]] Error& error()
    {
        return std::get<Error>(storage_);
    }

    [[nodiscard]] const Error& error() const
    {
        return std::get<Error>(storage_);
    }

    [[nodiscard]] T value_or(T fallback) const
    {
        if (is_ok()) {
            return value();
        }

        return fallback;
    }

    [[nodiscard]] static Result ok(T value)
    {
        return Result(std::move(value));
    }

    [[nodiscard]] static Result fail(Error error)
    {
        return Result(std::move(error));
    }

    [[nodiscard]] static Result fail(ErrorCode code, std::string message = {})
    {
        return Result(Error::make(code, std::move(message)));
    }

private:
    std::variant<T, Error> storage_;
};

template <>
class Result<void> {
public:
    Result()
        : error_(Error::none())
    {
    }

    Result(Error error)
        : error_(std::move(error))
    {
    }

    [[nodiscard]] bool is_ok() const
    {
        return error_.is_ok();
    }

    [[nodiscard]] bool is_error() const
    {
        return error_.is_error();
    }

    [[nodiscard]] explicit operator bool() const
    {
        return is_ok();
    }

    [[nodiscard]] Error& error()
    {
        return error_;
    }

    [[nodiscard]] const Error& error() const
    {
        return error_;
    }

    [[nodiscard]] static Result ok()
    {
        return Result();
    }

    [[nodiscard]] static Result fail(Error error)
    {
        return Result(std::move(error));
    }

    [[nodiscard]] static Result fail(ErrorCode code, std::string message = {})
    {
        return Result(Error::make(code, std::move(message)));
    }

private:
    Error error_;
};

}
