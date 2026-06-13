#pragma once

#include "graphics/common/GraphicsResult.h"

#include <cstdint>

namespace locus::graphics
{
    class GpuProfiler
    {
    public:
        GpuProfiler() = default;
        ~GpuProfiler();

        GpuProfiler(const GpuProfiler&) = delete;
        GpuProfiler& operator=(const GpuProfiler&) = delete;

        GpuProfiler(GpuProfiler&& other) noexcept;
        GpuProfiler& operator=(GpuProfiler&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create();
        void destroy();

        [[nodiscard]] GraphicsResult<void> begin();
        [[nodiscard]] GraphicsResult<void> end();

        [[nodiscard]] bool is_created() const;
        [[nodiscard]] bool is_running() const;
        [[nodiscard]] bool is_result_available() const;

        [[nodiscard]] GraphicsResult<std::uint64_t> result_nanoseconds() const;
        [[nodiscard]] GraphicsResult<double> result_microseconds() const;
        [[nodiscard]] GraphicsResult<double> result_milliseconds() const;

    private:
        unsigned int queryId_ = 0;
        bool running_ = false;
    };
}