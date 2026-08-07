/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/manufacturing/profiles/ManufacturingLimits.h"
#include "kernel/manufacturing/profiles/PrintTechnology.h"

#include <optional>
#include <string>

namespace locus::kernel::manufacturing {

    /**
     * @brief Manufacturing analysis profile for fused deposition modeling.
     *
     * The profile stores process characteristics relevant to geometric
     * printability analysis. It is not intended to represent a complete
     * slicer or printer configuration.
     */
    struct FDMProfile {
        /**
         * @brief Manufacturing technology represented by this profile.
         */
        static constexpr PrintTechnology technology = PrintTechnology::FDM;

        /**
         * @brief User-facing profile name.
         */
        std::string name{};

        /**
         * @brief Generic geometric manufacturing limits.
         */
        ManufacturingLimits limits{};

        /**
         * @brief Diameter of the extrusion nozzle.
         *
         * Uses the coordinate units of the analyzed model.
         */
        std::optional<double> nozzleDiameter{};

        /**
         * @brief Nominal deposited extrusion width.
         *
         * Uses the coordinate units of the analyzed model.
         */
        std::optional<double> extrusionWidth{};

        /**
         * @brief Nominal layer height.
         *
         * Uses the coordinate units of the analyzed model.
         */
        std::optional<double> layerHeight{};
    };

}