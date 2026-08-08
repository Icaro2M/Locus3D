/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/manufacturing/analyzers/thinwall/ThinWallQuality.h"
#include "kernel/manufacturing/core/IAnalyzer.h"

namespace locus::kernel::manufacturing {

    /**
     * @brief Specialized interface implemented by thin-wall analyzers.
     *
     * Thin-wall analyzers remain ordinary manufacturing IAnalyzer objects so
     * they can participate in AnalysisPipeline alongside topology, geometry,
     * and process analyzers.
     *
     * The additional quality() contract describes the analysis-quality mode
     * represented by the configured implementation.
     */
    class IThinWallAnalyzer : public IAnalyzer {
    public:
        /**
         * @brief Destroys a thin-wall analyzer.
         */
        ~IThinWallAnalyzer() override = default;

        /**
         * @brief Returns the quality mode represented by this analyzer.
         *
         * @return Configured thin-wall quality.
         */
        [[nodiscard]] virtual ThinWallQuality quality() const noexcept = 0;
    };

}