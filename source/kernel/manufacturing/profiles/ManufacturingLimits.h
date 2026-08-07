/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

namespace locus::kernel::manufacturing {

    /**
     * @brief Manufacturing limits used to evaluate printable geometry.
     *
     * Linear values use the coordinate units of the analyzed model.
     * Unset values indicate that the corresponding check does not have
     * a configured process limit.
     */
    struct ManufacturingLimits {
        /**
         * @brief Minimum printable wall thickness.
         */
        std::optional<double> minimumWallThickness{};

        /**
         * @brief Minimum printable geometric feature size.
         */
        std::optional<double> minimumFeatureSize{};

        /**
         * @brief Maximum unsupported overhang angle in degrees.
         *
         * When unset, no generic unsupported-overhang threshold is provided
         * by this profile.
         */
        std::optional<double> maximumUnsupportedOverhangAngleDegrees{};

        /**
         * @brief Checks whether a wall-thickness limit is configured.
         */
        [[nodiscard]] bool has_minimum_wall_thickness() const noexcept
        {
            return minimumWallThickness.has_value();
        }

        /**
         * @brief Checks whether a minimum-feature-size limit is configured.
         */
        [[nodiscard]] bool has_minimum_feature_size() const noexcept
        {
            return minimumFeatureSize.has_value();
        }

        /**
         * @brief Checks whether an unsupported-overhang limit is configured.
         */
        [[nodiscard]] bool has_maximum_unsupported_overhang_angle() const noexcept
        {
            return maximumUnsupportedOverhangAngleDegrees.has_value();
        }
    };

}