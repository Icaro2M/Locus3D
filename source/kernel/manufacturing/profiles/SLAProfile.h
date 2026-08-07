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
     * @brief Manufacturing analysis profile for resin-based
     * stereolithography-like processes.
     *
     * The profile intentionally models only geometric characteristics needed
     * by manufacturing analysis rather than complete machine exposure
     * settings.
     */
    struct SLAProfile {
        /**
         * @brief Manufacturing technology represented by this profile.
         */
        static constexpr PrintTechnology technology = PrintTechnology::SLA;

        /**
         * @brief User-facing profile name.
         */
        std::string name{};

        /**
         * @brief Generic geometric manufacturing limits.
         */
        ManufacturingLimits limits{};

        /**
         * @brief Effective process resolution in the XY plane.
         *
         * Uses the coordinate units of the analyzed model.
         */
        std::optional<double> xyResolution{};

        /**
         * @brief Nominal layer height.
         *
         * Uses the coordinate units of the analyzed model.
         */
        std::optional<double> layerHeight{};
    };

}