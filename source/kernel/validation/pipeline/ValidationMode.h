/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::kernel::validation {

    /**
     * @brief Validation execution depth.
     */
    enum class ValidationMode {
        /**
         * @brief Runs only cheap structural checks.
         */
        Quick,

        /**
         * @brief Runs the default validation set.
         */
        Standard,

        /**
         * @brief Runs all available validation checks.
         */
        Strict
    };

}