/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

namespace locus::kernel::manufacturing {

    /**
     * @brief Semantic kind of a numeric manufacturing measurement.
     *
     * Length-based values use the coordinate units of the analyzed model.
     * Conversion to presentation units belongs above the kernel.
     */
    enum class IssueMeasurementKind {
        /**
         * @brief Dimensionless numeric value.
         */
        Scalar,

        /**
         * @brief Linear measurement.
         */
        Length,

        /**
         * @brief Surface-area measurement.
         */
        Area,

        /**
         * @brief Volume measurement.
         */
        Volume,

        /**
         * @brief Angular measurement expressed in degrees.
         */
        AngleDegrees,

        /**
         * @brief Integral or conceptually discrete quantity.
         */
        Count,

        /**
         * @brief Dimensionless ratio.
         */
        Ratio
    };

    /**
     * @brief Numeric measurement associated with a manufacturing issue.
     */
    struct IssueMeasurement {
        /**
         * @brief Semantic interpretation of the numeric values.
         */
        IssueMeasurementKind kind = IssueMeasurementKind::Scalar;

        /**
         * @brief Value measured by the analyzer.
         */
        double value = 0.0;

        /**
         * @brief Relevant profile or analysis limit, when one exists.
         *
         * The comparison direction depends on the issue type. For example,
         * thin-wall values are problematic below their limit while overhang
         * angles are generally problematic above their limit.
         */
        std::optional<double> limit{};

        /**
         * @brief Checks whether a comparison limit is available.
         *
         * @return True when limit contains a value.
         */
        [[nodiscard]] bool has_limit() const
        {
            return limit.has_value();
        }
    };

}