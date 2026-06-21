/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Id.h"
#include "kernel/validation/core/ValidationSeverity.h"

#include <string>

namespace locus::kernel::validation {

    /**
     * @brief Single validation diagnostic reported by a validation check.
     */
    struct ValidationIssue {
        /**
         * @brief Diagnostic severity.
         */
        ValidationSeverity severity = ValidationSeverity::Error;

        /**
         * @brief Stable machine-readable issue code.
         */
        std::string code{};

        /**
         * @brief Name of the check that produced this issue.
         */
        std::string checkName{};

        /**
         * @brief Type of target element affected by the issue.
         */
        std::string targetType{};

        /**
         * @brief Identifier of the affected target element, when available.
         */
        Id targetId{};

        /**
         * @brief Human-readable diagnostic message.
         */
        std::string message{};
    };

}