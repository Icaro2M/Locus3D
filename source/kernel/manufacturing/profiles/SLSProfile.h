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
     * @brief Manufacturing analysis profile for selective laser sintering.
     *
     * Only parameters relevant to geometric printability analysis are stored
     * here. Full machine and material configuration belongs outside this
     * manufacturing-analysis contract.
     */
    struct SLSProfile {
        /**
         * @brief Manufacturing technology represented by this profile.
         */
        static constexpr PrintTechnology technology = PrintTechnology::SLS;

        /**
         * @brief User-facing profile name.
         */
        std::string name{};

        /**
         * @brief Generic geometric manufacturing limits.
         */
        ManufacturingLimits limits{};

        /**
         * @brief Effective laser spot diameter.
         *
         * Uses the coordinate units of the analyzed model.
         */
        std::optional<double> laserSpotDiameter{};

        /**
         * @brief Nominal layer height.
         *
         * Uses the coordinate units of the analyzed model.
         */
        std::optional<double> layerHeight{};
    };

}