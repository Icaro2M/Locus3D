/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::kernel::manufacturing {

    /**
     * @brief Additive manufacturing technology represented by a print profile.
     */
    enum class PrintTechnology {
        /**
         * @brief Fused deposition modeling / fused filament fabrication.
         */
        FDM,

        /**
         * @brief Vat photopolymerization using stereolithography-like
         * processes.
         */
        SLA,

        /**
         * @brief Powder-bed fusion using selective laser sintering.
         */
        SLS
    };

}