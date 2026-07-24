/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace locus::application {

    /**
     * @brief Immutable-by-convention timing data for one application frame.
     */
    struct FrameContext {
        /**
         * @brief Zero-based index of the current frame.
         */
        std::uint64_t frameIndex = 0;

        /**
         * @brief Frame delta after applying the configured maximum.
         */
        double deltaSeconds = 0.0;

        /**
         * @brief Unclamped elapsed time since the preceding frame.
         */
        double rawDeltaSeconds = 0.0;

        /**
         * @brief Monotonic elapsed time since the clock was reset.
         */
        double totalTimeSeconds = 0.0;

        /**
         * @brief True only for the first frame after construction or reset.
         */
        bool firstFrame = true;
    };

} // namespace locus::application
