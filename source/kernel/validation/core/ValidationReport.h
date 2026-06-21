/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/validation/core/ValidationIssue.h"
#include "kernel/validation/core/ValidationSeverity.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace locus::kernel::validation {

    /**
     * @brief Collection of validation diagnostics produced by one or more checks.
     */
    class ValidationReport {
    public:
        /**
         * @brief Adds a validation issue to the report.
         *
         * @param issue Issue to append.
         */
        void add_issue(ValidationIssue issue)
        {
            issues_.push_back(std::move(issue));
        }

        /**
         * @brief Adds a validation issue to the report.
         *
         * @param severity Diagnostic severity.
         * @param code Machine-readable issue code.
         * @param checkName Name of the check that produced this issue.
         * @param targetType Type of target element affected by the issue.
         * @param targetId Identifier of the affected target element.
         * @param message Human-readable diagnostic message.
         */
        void add_issue(
            ValidationSeverity severity,
            std::string code,
            std::string checkName,
            std::string targetType,
            Id targetId,
            std::string message)
        {
            issues_.push_back(ValidationIssue{
                severity,
                std::move(code),
                std::move(checkName),
                std::move(targetType),
                targetId,
                std::move(message)
                });
        }

        /**
         * @brief Appends all issues from another report.
         *
         * @param other Report to merge into this one.
         */
        void merge(const ValidationReport& other)
        {
            issues_.insert(issues_.end(), other.issues_.begin(), other.issues_.end());
        }

        /**
         * @brief Checks whether the report contains no error diagnostics.
         *
         * @return True when no issue has Error severity.
         */
        [[nodiscard]] bool valid() const
        {
            return error_count() == 0;
        }

        /**
         * @brief Checks whether the report contains any diagnostics.
         *
         * @return True when at least one issue was reported.
         */
        [[nodiscard]] bool has_issues() const
        {
            return !issues_.empty();
        }

        /**
         * @brief Checks whether the report contains error diagnostics.
         *
         * @return True when at least one issue has Error severity.
         */
        [[nodiscard]] bool has_errors() const
        {
            return error_count() > 0;
        }

        /**
         * @brief Checks whether the report contains warning diagnostics.
         *
         * @return True when at least one issue has Warning severity.
         */
        [[nodiscard]] bool has_warnings() const
        {
            return warning_count() > 0;
        }

        /**
         * @brief Returns the number of diagnostics.
         *
         * @return Total issue count.
         */
        [[nodiscard]] std::size_t issue_count() const
        {
            return issues_.size();
        }

        /**
         * @brief Counts diagnostics with Info severity.
         *
         * @return Number of informational issues.
         */
        [[nodiscard]] std::size_t info_count() const
        {
            return count_severity(ValidationSeverity::Info);
        }

        /**
         * @brief Counts diagnostics with Warning severity.
         *
         * @return Number of warning issues.
         */
        [[nodiscard]] std::size_t warning_count() const
        {
            return count_severity(ValidationSeverity::Warning);
        }

        /**
         * @brief Counts diagnostics with Error severity.
         *
         * @return Number of error issues.
         */
        [[nodiscard]] std::size_t error_count() const
        {
            return count_severity(ValidationSeverity::Error);
        }

        /**
         * @brief Returns all validation issues.
         *
         * @return Read-only issue list.
         */
        [[nodiscard]] const std::vector<ValidationIssue>& issues() const
        {
            return issues_;
        }

        /**
         * @brief Removes all diagnostics from the report.
         */
        void clear()
        {
            issues_.clear();
        }

    private:
        [[nodiscard]] std::size_t count_severity(ValidationSeverity severity) const
        {
            std::size_t count = 0;

            for (const ValidationIssue& issue : issues_) {
                if (issue.severity == severity) {
                    ++count;
                }
            }

            return count;
        }

        std::vector<ValidationIssue> issues_{};
    };

}