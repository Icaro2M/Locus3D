/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/manufacturing/core/IssueLocation.h"
#include "kernel/manufacturing/core/IssueMeasurement.h"
#include "kernel/manufacturing/core/IssueSeverity.h"
#include "kernel/manufacturing/core/PrintIssueType.h"

#include <optional>
#include <string>
#include <utility>

namespace locus::kernel::manufacturing {

    /**
     * @brief Manufacturing problem or diagnostic discovered by an analyzer.
     *
     * A print issue stores semantic information independently from editor,
     * viewport, graphics, or user-interface presentation.
     */
    struct PrintIssue {
        /**
         * @brief Creates a manufacturing issue.
         *
         * @param issueType Semantic issue category.
         * @param issueSeverity Manufacturing severity.
         * @param issueMessage Human-readable diagnostic message.
         */
        PrintIssue(
            PrintIssueType issueType,
            IssueSeverity issueSeverity,
            std::string issueMessage)
            : type(issueType),
            severity(issueSeverity),
            message(std::move(issueMessage))
        {
        }

        /**
         * @brief Creates a manufacturing issue with an affected location.
         *
         * @param issueType Semantic issue category.
         * @param issueSeverity Manufacturing severity.
         * @param issueMessage Human-readable diagnostic message.
         * @param issueLocation Affected mesh or spatial region.
         */
        PrintIssue(
            PrintIssueType issueType,
            IssueSeverity issueSeverity,
            std::string issueMessage,
            IssueLocation issueLocation)
            : type(issueType),
            severity(issueSeverity),
            message(std::move(issueMessage)),
            location(std::move(issueLocation))
        {
        }

        /**
         * @brief Semantic category of the issue.
         */
        PrintIssueType type;

        /**
         * @brief Manufacturing severity of the issue.
         */
        IssueSeverity severity;

        /**
         * @brief Human-readable diagnostic message.
         *
         * The message is supplemental information. Consumers should use
         * PrintIssueType rather than parse this text to determine behavior.
         */
        std::string message{};

        /**
         * @brief Editable-mesh and spatial region affected by the issue.
         */
        IssueLocation location{};

        /**
         * @brief Numeric measurement associated with the issue, when relevant.
         *
         * Examples include wall thickness, minimum feature size, overhang
         * angle, or another value meaningful to the analyzer.
         */
        std::optional<IssueMeasurement> measurement{};

        /**
         * @brief Checks whether the issue contains localization information.
         *
         * @return True when location is not empty.
         */
        [[nodiscard]] bool has_location() const
        {
            return !location.empty();
        }

        /**
         * @brief Checks whether a numeric measurement is available.
         *
         * @return True when measurement contains a value.
         */
        [[nodiscard]] bool has_measurement() const
        {
            return measurement.has_value();
        }
    };

}