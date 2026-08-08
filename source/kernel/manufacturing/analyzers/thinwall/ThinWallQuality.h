/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::kernel::manufacturing {

    /**
     * @brief Requested quality level for thin-wall analysis.
     *
     * Quality expresses the desired tradeoff between analysis cost and
     * geometric accuracy. It intentionally does not expose implementation
     * details such as CPU, GPU, ray count, or acceleration backend.
     */
    enum class ThinWallQuality {
        /**
         * @brief Low-cost approximate analysis intended for rapid feedback.
         */
        Fast,

        /**
         * @brief Balanced analysis suitable for ordinary interactive checks.
         */
        Balanced,

        /**
         * @brief Higher-cost analysis intended to maximize geometric accuracy.
         */
        High
    };

}