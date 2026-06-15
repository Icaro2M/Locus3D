/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/debug/GpuProfiler.h"

#include <glad/glad.h>

#include <utility>

namespace locus::graphics
{
    GpuProfiler::~GpuProfiler()
    {
        destroy();
    }

    GpuProfiler::GpuProfiler(GpuProfiler&& other) noexcept
    {
        queryId_ = std::exchange(other.queryId_, 0);
        running_ = std::exchange(other.running_, false);
    }

    GpuProfiler& GpuProfiler::operator=(GpuProfiler&& other) noexcept
    {
        if (this != &other)
        {
            destroy();

            queryId_ = std::exchange(other.queryId_, 0);
            running_ = std::exchange(other.running_, false);
        }

        return *this;
    }

    GraphicsResult<void> GpuProfiler::create()
    {
        destroy();

        glGenQueries(1, &queryId_);

        if (queryId_ == 0)
        {
            return GraphicsError{
                GraphicsErrorCode::InvalidOperation,
                "Failed to create GPU profiler query."
            };
        }

        running_ = false;

        return {};
    }

    void GpuProfiler::destroy()
    {
        if (queryId_ == 0)
        {
            running_ = false;
            return;
        }

        if (running_)
        {
            // Close an active query before deletion to keep OpenGL state balanced.
            glEndQuery(GL_TIME_ELAPSED);
            running_ = false;
        }

        glDeleteQueries(1, &queryId_);
        queryId_ = 0;
    }

    GraphicsResult<void> GpuProfiler::begin()
    {
        if (queryId_ == 0)
        {
            return GraphicsError{
                GraphicsErrorCode::InvalidOperation,
                "GpuProfiler must be created before begin()."
            };
        }

        if (running_)
        {
            return GraphicsError{
                GraphicsErrorCode::InvalidOperation,
                "GpuProfiler query is already running."
            };
        }

        glBeginQuery(GL_TIME_ELAPSED, queryId_);
        running_ = true;

        return {};
    }

    GraphicsResult<void> GpuProfiler::end()
    {
        if (queryId_ == 0)
        {
            return GraphicsError{
                GraphicsErrorCode::InvalidOperation,
                "GpuProfiler must be created before end()."
            };
        }

        if (!running_)
        {
            return GraphicsError{
                GraphicsErrorCode::InvalidOperation,
                "GpuProfiler query is not running."
            };
        }

        glEndQuery(GL_TIME_ELAPSED);
        running_ = false;

        return {};
    }

    bool GpuProfiler::is_created() const
    {
        return queryId_ != 0;
    }

    bool GpuProfiler::is_running() const
    {
        return running_;
    }

    bool GpuProfiler::is_result_available() const
    {
        if (queryId_ == 0 || running_)
        {
            return false;
        }

        int available = 0;
        glGetQueryObjectiv(queryId_, GL_QUERY_RESULT_AVAILABLE, &available);

        return available == GL_TRUE;
    }

    GraphicsResult<std::uint64_t> GpuProfiler::result_nanoseconds() const
    {
        if (queryId_ == 0)
        {
            return GraphicsError{
                GraphicsErrorCode::InvalidOperation,
                "GpuProfiler must be created before reading results."
            };
        }

        if (running_)
        {
            return GraphicsError{
                GraphicsErrorCode::InvalidOperation,
                "GpuProfiler result cannot be read while the query is running."
            };
        }

        unsigned long long elapsed = 0;
        glGetQueryObjectui64v(queryId_, GL_QUERY_RESULT, &elapsed);

        return static_cast<std::uint64_t>(elapsed);
    }

    GraphicsResult<double> GpuProfiler::result_microseconds() const
    {
        auto result = result_nanoseconds();

        if (!result)
        {
            return result.error();
        }

        return static_cast<double>(result.value()) / 1000.0;
    }

    GraphicsResult<double> GpuProfiler::result_milliseconds() const
    {
        auto result = result_nanoseconds();

        if (!result)
        {
            return result.error();
        }

        return static_cast<double>(result.value()) / 1000000.0;
    }
}
