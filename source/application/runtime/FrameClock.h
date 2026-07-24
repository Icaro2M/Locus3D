/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/runtime/FrameContext.h"

#include <chrono>
#include <cstdint>

namespace locus::application {

    /**
     * @brief Measures monotonic frame timing independently of platform systems.
     */
    class FrameClock {
    public:
        using Clock = std::chrono::steady_clock;
        using TimePoint = Clock::time_point;

        /**
         * @brief Creates and starts a frame clock.
         *
         * @param maximumDeltaSeconds Maximum delta returned to consumers.
         */
        explicit FrameClock(double maximumDeltaSeconds = 0.25) noexcept;

        /**
         * @brief Restarts timing at the current monotonic time.
         */
        void reset() noexcept;

        /**
         * @brief Restarts timing at a supplied monotonic instant.
         *
         * @param now New timing origin.
         */
        void reset(TimePoint now) noexcept;

        /**
         * @brief Measures the next frame at the current monotonic time.
         *
         * @return Timing context for the measured frame.
         */
        [[nodiscard]] FrameContext tick() noexcept;

        /**
         * @brief Measures the next frame at a supplied monotonic instant.
         *
         * This overload supports deterministic tests without a platform clock.
         *
         * @param now Monotonic instant representing the frame boundary.
         * @return Timing context for the measured frame.
         */
        [[nodiscard]] FrameContext tick(TimePoint now) noexcept;

        /**
         * @brief Changes the maximum delta returned to frame consumers.
         *
         * Negative values are normalized to zero.
         *
         * @param seconds Maximum frame delta in seconds.
         */
        void set_maximum_delta(double seconds) noexcept;

        /**
         * @brief Returns the configured maximum frame delta.
         *
         * @return Maximum delta in seconds.
         */
        [[nodiscard]] double maximum_delta() const noexcept;

        /**
         * @brief Returns the index assigned to the next frame.
         *
         * @return Next zero-based frame index.
         */
        [[nodiscard]] std::uint64_t next_frame_index() const noexcept;

    private:
        TimePoint startTime_{};
        TimePoint previousTime_{};
        double maximumDeltaSeconds_ = 0.25;
        std::uint64_t nextFrameIndex_ = 0;
        bool firstFrame_ = true;
    };

} // namespace locus::application
