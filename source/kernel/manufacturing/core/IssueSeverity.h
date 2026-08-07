/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::kernel::manufacturing {

    /**
     * @brief Severity assigned to a manufacturing analysis issue.
     *
     * Severity describes the expected impact of an issue on manufacturing.
     * It does not indicate whether the editable LEM itself is structurally
     * valid; internal mesh validity is handled by kernel validation.
     */
    enum class IssueSeverity {
        /**
         * @brief Informational finding that does not normally prevent printing.
         */
        Info,

        /**
         * @brief Potential manufacturing problem that should be reviewed.
         *
         * The model may still be printable depending on the selected process,
         * printer capabilities, material, orientation, or user tolerance.
         */
        Warning,

        /**
         * @brief Manufacturing problem expected to prevent or seriously
         * compromise printing with the active profile.
         */
        Error
    };

}