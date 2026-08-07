/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/manufacturing/core/AnalysisContext.h"
#include "kernel/manufacturing/core/AnalysisReport.h"

#include <string_view>

namespace locus::kernel::manufacturing {

    /**
     * @brief Common interface implemented by manufacturing analyzers.
     *
     * An analyzer inspects manufacturing-related properties of geometry and
     * appends its findings to an AnalysisReport. It must not modify the
     * editable mesh, print profile, or derived analysis representation.
     */
    class IAnalyzer {
    public:
        /**
         * @brief Destroys a manufacturing analyzer.
         */
        virtual ~IAnalyzer() = default;

        /**
         * @brief Returns the stable analyzer name.
         *
         * The name identifies the analyzer for diagnostics, pipelines, tests,
         * and future presentation or configuration systems. Consumer behavior
         * must not depend on parsing this string.
         *
         * @return Stable analyzer name.
         */
        [[nodiscard]] virtual std::string_view name() const noexcept = 0;

        /**
         * @brief Executes the manufacturing analysis.
         *
         * Produced issues and metrics are appended to the supplied report.
         * Existing report contents must be preserved.
         *
         * Individual analyzers are responsible only for their own analysis;
         * orchestration and execution ordering belong to AnalysisPipeline.
         *
         * @param context Read-only manufacturing analysis inputs.
         * @param report Report that receives generated findings and metrics.
         */
        virtual void analyze(
            const AnalysisContext& context,
            AnalysisReport& report) const = 0;
    };

}