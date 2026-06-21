/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::kernel::validation {

    /**
     * @brief Severity assigned to a validation diagnostic.
     */
    enum class ValidationSeverity {
        /**
         * @brief Informational diagnostic that does not indicate an invalid state.
         */
        Info,

        /**
         * @brief Suspicious state that may still be usable.
         */
        Warning,

        /**
         * @brief Invalid state that should be fixed before continuing.
         */
        Error
    };

}