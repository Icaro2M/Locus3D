/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::kernel::geometry {

    class LEM;

}

namespace locus::kernel::manufacturing {

    class AnalysisMesh;
    class PrintProfile;

    /**
     * @brief Input data shared by manufacturing analyzers.
     *
     * The editable LEM remains the authoritative geometry source. A derived
     * AnalysisMesh may optionally be supplied for analyzers that require
     * triangulated or accelerated geometric data.
     *
     * The print profile is optional at the context level because some
     * topology and geometry analyzers are independent from manufacturing
     * technology or process limits.
     */
    struct AnalysisContext {
        /**
         * @brief Authoritative editable mesh being analyzed.
         */
        const geometry::LEM* mesh = nullptr;

        /**
         * @brief Active manufacturing profile, when the analysis depends on
         * process-specific limits or technology characteristics.
         */
        const PrintProfile* profile = nullptr;

        /**
         * @brief Derived analysis representation, when available.
         *
         * This object is expected to be rebuilt from the LEM when geometry
         * changes. It must never become the authoritative editable mesh.
         */
        const AnalysisMesh* analysisMesh = nullptr;

        /**
         * @brief Checks whether an editable mesh is available.
         *
         * @return True when mesh references a LEM.
         */
        [[nodiscard]] bool has_mesh() const noexcept
        {
            return mesh != nullptr;
        }

        /**
         * @brief Checks whether a manufacturing profile is available.
         *
         * @return True when profile references a PrintProfile.
         */
        [[nodiscard]] bool has_profile() const noexcept
        {
            return profile != nullptr;
        }

        /**
         * @brief Checks whether a derived analysis mesh is available.
         *
         * @return True when analysisMesh references an AnalysisMesh.
         */
        [[nodiscard]] bool has_analysis_mesh() const noexcept
        {
            return analysisMesh != nullptr;
        }
    };

}