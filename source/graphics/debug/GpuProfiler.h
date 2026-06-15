/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"

#include <cstdint>

namespace locus::graphics
{
    /**
     * @brief Measures GPU elapsed time with an OpenGL timer query.
     *
     * GpuProfiler owns one GL_TIME_ELAPSED query object and exposes results in
     * nanoseconds, microseconds, or milliseconds after the query completes.
     *
     * @note Timer query results may become available one or more frames later.
     */
    class GpuProfiler
    {
    public:
        GpuProfiler() = default;
        ~GpuProfiler();

        GpuProfiler(const GpuProfiler&) = delete;
        GpuProfiler& operator=(const GpuProfiler&) = delete;

        GpuProfiler(GpuProfiler&& other) noexcept;
        GpuProfiler& operator=(GpuProfiler&& other) noexcept;

        /**
         * @brief Creates the OpenGL timer query object.
         *
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create();

        /**
         * @brief Destroys the timer query object.
         */
        void destroy();

        /**
         * @brief Begins measuring elapsed GPU time.
         *
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> begin();

        /**
         * @brief Ends the active GPU timing query.
         *
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> end();

        /**
         * @brief Checks whether a query object exists.
         *
         * @return True when the profiler owns a query id.
         */
        [[nodiscard]] bool is_created() const;

        /**
         * @brief Checks whether a timing query is currently active.
         *
         * @return True between begin() and end().
         */
        [[nodiscard]] bool is_running() const;

        /**
         * @brief Checks whether the last timing result can be read without blocking.
         *
         * @return True when OpenGL reports the result as available.
         */
        [[nodiscard]] bool is_result_available() const;

        /**
         * @brief Reads the last elapsed GPU time in nanoseconds.
         *
         * @return Elapsed nanoseconds, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<std::uint64_t> result_nanoseconds() const;

        /**
         * @brief Reads the last elapsed GPU time in microseconds.
         *
         * @return Elapsed microseconds, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<double> result_microseconds() const;

        /**
         * @brief Reads the last elapsed GPU time in milliseconds.
         *
         * @return Elapsed milliseconds, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<double> result_milliseconds() const;

    private:
        unsigned int queryId_ = 0;
        bool running_ = false;
    };
}
