#pragma once

#include "GraphicsError.h"

#include <utility>

namespace locus::graphics
{

    template <typename T>
    class GraphicsResult
    {
    public:
        GraphicsResult(const T& value)
            : value_(value), error_(GraphicsError::none()), has_value_(true)
        {
        }

        GraphicsResult(T&& value)
            : value_(std::move(value)), error_(GraphicsError::none()), has_value_(true)
        {
        }

        GraphicsResult(GraphicsError error)
            : value_(), error_(std::move(error)), has_value_(false)
        {
        }

        [[nodiscard]] bool ok() const
        {
            return has_value_;
        }

        [[nodiscard]] explicit operator bool() const
        {
            return ok();
        }

        [[nodiscard]] const T& value() const
        {
            return value_;
        }

        [[nodiscard]] T& value()
        {
            return value_;
        }

        [[nodiscard]] T&& move_value()
        {
            return std::move(value_);
        }

        [[nodiscard]] const GraphicsError& error() const
        {
            return error_;
        }

    private:
        T value_{};
        GraphicsError error_{};
        bool has_value_ = false;
    };

    template <>
    class GraphicsResult<void>
    {
    public:
        GraphicsResult()
            : error_(GraphicsError::none()), success_(true)
        {
        }

        GraphicsResult(GraphicsError error)
            : error_(std::move(error)), success_(false)
        {
        }

        [[nodiscard]] bool ok() const
        {
            return success_;
        }

        [[nodiscard]] explicit operator bool() const
        {
            return ok();
        }

        [[nodiscard]] const GraphicsError& error() const
        {
            return error_;
        }

    private:
        GraphicsError error_{};
        bool success_ = true;
    };

}