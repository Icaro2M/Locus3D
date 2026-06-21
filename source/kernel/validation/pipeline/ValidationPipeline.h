/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/validation/core/IValidationCheck.h"
#include "kernel/validation/core/ValidationContext.h"
#include "kernel/validation/core/ValidationReport.h"
#include "kernel/validation/pipeline/ValidationMode.h"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace locus::kernel::validation {

    /**
     * @brief Ordered collection of validation checks.
     */
    class ValidationPipeline {
    public:
        /**
         * @brief Creates a validation pipeline.
         */
        ValidationPipeline() = default;

        /**
         * @brief Destroys the validation pipeline.
         */
        ~ValidationPipeline() = default;

        ValidationPipeline(const ValidationPipeline&) = delete;
        ValidationPipeline& operator=(const ValidationPipeline&) = delete;

        ValidationPipeline(ValidationPipeline&&) noexcept = default;
        ValidationPipeline& operator=(ValidationPipeline&&) noexcept = default;

        /**
         * @brief Adds a validation check to the pipeline.
         *
         * @param check Check instance to append.
         */
        void add_check(std::unique_ptr<IValidationCheck> check)
        {
            if (check) {
                checks_.push_back(std::move(check));
            }
        }

        /**
         * @brief Runs all checks in insertion order.
         *
         * @param context Validation input data.
         * @return Validation report produced by all checks.
         */
        [[nodiscard]] ValidationReport validate(const ValidationContext& context) const
        {
            ValidationReport report{};

            for (const std::unique_ptr<IValidationCheck>& check : checks_) {
                check->validate(context, report);
            }

            return report;
        }

        /**
         * @brief Removes all checks from the pipeline.
         */
        void clear()
        {
            checks_.clear();
        }

        /**
         * @brief Returns the number of checks registered in the pipeline.
         *
         * @return Registered check count.
         */
        [[nodiscard]] std::size_t check_count() const
        {
            return checks_.size();
        }

        /**
         * @brief Returns the configured validation mode.
         *
         * @return Current validation mode.
         */
        [[nodiscard]] ValidationMode mode() const
        {
            return mode_;
        }

        /**
         * @brief Updates the configured validation mode.
         *
         * @param mode New validation mode.
         */
        void set_mode(ValidationMode mode)
        {
            mode_ = mode;
        }

    private:
        ValidationMode mode_ = ValidationMode::Standard;
        std::vector<std::unique_ptr<IValidationCheck>> checks_{};
    };

}