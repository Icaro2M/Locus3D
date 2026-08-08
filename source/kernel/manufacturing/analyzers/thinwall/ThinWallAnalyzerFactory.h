/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/manufacturing/analyzers/thinwall/IThinWallAnalyzer.h"
#include "kernel/manufacturing/analyzers/thinwall/RaycastThinWallAnalyzer.h"
#include "kernel/manufacturing/analyzers/thinwall/ThinWallQuality.h"

#include <memory>

namespace locus::kernel::manufacturing {

    /**
     * @brief Creates configured thin-wall analyzer implementations.
     *
     * The factory decouples callers from the concrete algorithm used for each
     * ThinWallQuality. This allows manufacturing internals to change the
     * backend associated with a quality level without affecting AnalysisPipeline
     * or other consumers.
     */
    class ThinWallAnalyzerFactory {
    public:
        /**
         * @brief Creates a thin-wall analyzer for the requested quality.
         *
         * The current implementation uses BVH-accelerated raycast analysis for
         * all quality levels. Sampling density and analysis cost are selected
         * internally by RaycastThinWallAnalyzer.
         *
         * @param quality Requested analysis quality.
         * @return Owning pointer to the configured analyzer.
         */
        [[nodiscard]] static std::unique_ptr<IThinWallAnalyzer> create(
            ThinWallQuality quality)
        {
            return std::make_unique<RaycastThinWallAnalyzer>(
                quality);
        }
    };

}