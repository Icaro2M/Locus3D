/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/validation/core/ValidationContext.h"
#include "kernel/validation/core/ValidationReport.h"

#include <string_view>

namespace locus::kernel::validation {

    /**
     * @brief Interface implemented by validation checks.
     */
    class IValidationCheck {
    public:
        /**
         * @brief Destroys a validation check.
         */
        virtual ~IValidationCheck() = default;

        /**
         * @brief Returns the stable check name.
         *
         * @return Check name used in diagnostics.
         */
        [[nodiscard]] virtual std::string_view name() const = 0;

        /**
         * @brief Executes the validation check.
         *
         * @param context Validation input data.
         * @param report Report that receives produced diagnostics.
         */
        virtual void validate(const ValidationContext& context, ValidationReport& report) const = 0;
    };

}