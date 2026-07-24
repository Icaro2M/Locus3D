/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/runtime/FrameClock.h"

#include <algorithm>

namespace locus::application {

    FrameClock::FrameClock(double maximumDeltaSeconds) noexcept
    {
        set_maximum_delta(maximumDeltaSeconds);
        reset();
    }

    void FrameClock::reset() noexcept
    {
        reset(Clock::now());
    }

    void FrameClock::reset(TimePoint now) noexcept
    {
        startTime_ = now;
        previousTime_ = now;
        nextFrameIndex_ = 0;
        firstFrame_ = true;
    }

    FrameContext FrameClock::tick() noexcept
    {
        return tick(Clock::now());
    }

    FrameContext FrameClock::tick(TimePoint now) noexcept
    {
        const TimePoint effectiveNow = std::max(now, previousTime_);
        const double totalTimeSeconds =
            std::chrono::duration<double>(effectiveNow - startTime_).count();

        double rawDeltaSeconds = 0.0;
        if (!firstFrame_) {
            rawDeltaSeconds =
                std::chrono::duration<double>(effectiveNow - previousTime_).count();
        }

        const double deltaSeconds =
            std::min(rawDeltaSeconds, maximumDeltaSeconds_);

        const FrameContext context{
            nextFrameIndex_,
            deltaSeconds,
            rawDeltaSeconds,
            totalTimeSeconds,
            firstFrame_
        };

        previousTime_ = effectiveNow;
        ++nextFrameIndex_;
        firstFrame_ = false;
        return context;
    }

    void FrameClock::set_maximum_delta(double seconds) noexcept
    {
        maximumDeltaSeconds_ = std::max(seconds, 0.0);
    }

    double FrameClock::maximum_delta() const noexcept
    {
        return maximumDeltaSeconds_;
    }

    std::uint64_t FrameClock::next_frame_index() const noexcept
    {
        return nextFrameIndex_;
    }

} // namespace locus::application
