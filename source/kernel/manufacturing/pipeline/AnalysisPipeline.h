/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/manufacturing/core/AnalysisReport.h"
#include "kernel/manufacturing/pipeline/AnalysisOptions.h"

namespace locus::kernel::geometry {
    class LEM;
}

namespace locus::kernel::manufacturing {

    class PrintProfile;

    /**
     * @brief Executes the complete manufacturing-analysis workflow.
     *
     * AnalysisPipeline owns orchestration only. Individual geometric and
     * process rules remain implemented by specialized IAnalyzer objects.
     *
     * One canonical AnalysisMesh is built per pipeline execution and shared by
     * every analyzer that requires derived triangulation or spatial queries.
     */
    class AnalysisPipeline {
    public:
        /**
         * @brief Analyzes a mesh without a manufacturing profile.
         *
         * Profile-dependent analyses such as minimum feature size, thin wall,
         * and overhang are naturally skipped.
         *
         * @param mesh Source editable mesh.
         * @param options Analysis configuration.
         * @return Consolidated manufacturing report.
         */
        [[nodiscard]] static AnalysisReport analyze(
            const geometry::LEM& mesh,
            const AnalysisOptions& options = {});

        /**
         * @brief Analyzes a mesh using a manufacturing profile.
         *
         * @param mesh Source editable mesh.
         * @param profile Active print profile.
         * @param options Analysis configuration.
         * @return Consolidated manufacturing report.
         */
        [[nodiscard]] static AnalysisReport analyze(
            const geometry::LEM& mesh,
            const PrintProfile& profile,
            const AnalysisOptions& options = {});

        /**
         * @brief Executes analysis into an existing report.
         *
         * The destination report is cleared before execution because a
         * pipeline run represents a complete analysis snapshot.
         *
         * @param mesh Source editable mesh.
         * @param report Destination report.
         * @param options Analysis configuration.
         */
        static void analyze_into(
            const geometry::LEM& mesh,
            AnalysisReport& report,
            const AnalysisOptions& options = {});

        /**
         * @brief Executes profiled analysis into an existing report.
         *
         * The destination report is cleared before execution.
         *
         * @param mesh Source editable mesh.
         * @param profile Active print profile.
         * @param report Destination report.
         * @param options Analysis configuration.
         */
        static void analyze_into(
            const geometry::LEM& mesh,
            const PrintProfile& profile,
            AnalysisReport& report,
            const AnalysisOptions& options = {});

    private:
        static void execute(
            const geometry::LEM& mesh,
            const PrintProfile* profile,
            AnalysisReport& report,
            const AnalysisOptions& options);
    };

}