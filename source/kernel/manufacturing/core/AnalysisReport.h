/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/manufacturing/core/AnalysisMetrics.h"
#include "kernel/manufacturing/core/IssueSeverity.h"
#include "kernel/manufacturing/core/PrintIssue.h"
#include "kernel/manufacturing/core/PrintIssueType.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Aggregated result of manufacturing analysis.
     *
     * A report contains user-facing manufacturing issues together with
     * non-diagnostic geometric metrics produced during analysis.
     */
    class AnalysisReport {
    public:
        /**
         * @brief Adds a manufacturing issue to the report.
         *
         * @param issue Issue to append.
         */
        void add_issue(PrintIssue issue)
        {
            issues_.push_back(std::move(issue));
        }

        /**
         * @brief Checks whether any issues were reported.
         *
         * @return True when the report contains at least one issue.
         */
        [[nodiscard]] bool has_issues() const
        {
            return !issues_.empty();
        }

        /**
         * @brief Checks whether at least one error issue exists.
         *
         * @return True when an Error issue is present.
         */
        [[nodiscard]] bool has_errors() const
        {
            return error_count() > 0;
        }

        /**
         * @brief Checks whether at least one warning issue exists.
         *
         * @return True when a Warning issue is present.
         */
        [[nodiscard]] bool has_warnings() const
        {
            return warning_count() > 0;
        }

        /**
         * @brief Checks whether an issue of the specified semantic type exists.
         *
         * @param type Issue type to find.
         * @return True when at least one matching issue is present.
         */
        [[nodiscard]] bool has_issue_type(PrintIssueType type) const
        {
            return issue_count(type) > 0;
        }

        /**
         * @brief Returns the total number of issues.
         *
         * @return Number of issues stored in the report.
         */
        [[nodiscard]] std::size_t issue_count() const
        {
            return issues_.size();
        }

        /**
         * @brief Counts issues with the specified type.
         *
         * @param type Issue type to count.
         * @return Number of matching issues.
         */
        [[nodiscard]] std::size_t issue_count(PrintIssueType type) const
        {
            std::size_t count = 0;

            for (const PrintIssue& issue : issues_) {
                if (issue.type == type) {
                    ++count;
                }
            }

            return count;
        }

        /**
         * @brief Counts informational issues.
         *
         * @return Number of Info issues.
         */
        [[nodiscard]] std::size_t info_count() const
        {
            return count_severity(IssueSeverity::Info);
        }

        /**
         * @brief Counts warning issues.
         *
         * @return Number of Warning issues.
         */
        [[nodiscard]] std::size_t warning_count() const
        {
            return count_severity(IssueSeverity::Warning);
        }

        /**
         * @brief Counts error issues.
         *
         * @return Number of Error issues.
         */
        [[nodiscard]] std::size_t error_count() const
        {
            return count_severity(IssueSeverity::Error);
        }

        /**
         * @brief Returns all manufacturing issues.
         *
         * @return Read-only issue collection.
         */
        [[nodiscard]] const std::vector<PrintIssue>& issues() const
        {
            return issues_;
        }

        /**
         * @brief Returns read-only analysis metrics.
         *
         * @return Analysis metrics.
         */
        [[nodiscard]] const AnalysisMetrics& metrics() const
        {
            return metrics_;
        }

        /**
         * @brief Returns mutable analysis metrics for analyzers and pipelines.
         *
         * @return Mutable analysis metrics.
         */
        [[nodiscard]] AnalysisMetrics& metrics()
        {
            return metrics_;
        }

        /**
         * @brief Removes all issues and computed metrics.
         */
        void clear()
        {
            issues_.clear();
            metrics_.clear();
        }

    private:
        /**
         * @brief Counts issues with a specified severity.
         *
         * @param severity Severity to count.
         * @return Number of matching issues.
         */
        [[nodiscard]] std::size_t count_severity(
            IssueSeverity severity) const
        {
            std::size_t count = 0;

            for (const PrintIssue& issue : issues_) {
                if (issue.severity == severity) {
                    ++count;
                }
            }

            return count;
        }

        std::vector<PrintIssue> issues_{};
        AnalysisMetrics metrics_{};
    };

}