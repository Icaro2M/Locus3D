/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/manufacturing/ManufacturingDisplaySettings.h"

namespace locus::editor {

    namespace {

        [[nodiscard]] int severity_rank(
            kernel::manufacturing::IssueSeverity severity) noexcept
        {
            using kernel::manufacturing::IssueSeverity;

            switch (severity) {
            case IssueSeverity::Info:
                return 0;
            case IssueSeverity::Warning:
                return 1;
            case IssueSeverity::Error:
                return 2;
            }

            return 0;
        }

        [[nodiscard]] ManufacturingVisualStyle make_style(
            graphics::ColorRGBA color,
            int priority,
            float surfaceAlpha = 0.34f,
            float lineWidthPixels = 4.0f,
            float markerRadiusPixels = 7.0f) noexcept
        {
            ManufacturingVisualStyle style{};
            style.color = color;
            style.surfaceAlpha = surfaceAlpha;
            style.lineWidthPixels = lineWidthPixels;
            style.markerRadiusPixels = markerRadiusPixels;
            style.priority = priority;
            return style;
        }

    } // namespace

    bool ManufacturingDisplaySettings::is_type_visible(
        kernel::manufacturing::PrintIssueType type) const noexcept
    {
        using kernel::manufacturing::PrintIssueType;

        switch (type) {
        case PrintIssueType::LooseEdge:
            return issueVisibility.looseEdges;
        case PrintIssueType::OpenBoundary:
            return issueVisibility.openBoundaries;
        case PrintIssueType::NonManifoldEdge:
            return issueVisibility.nonManifoldEdges;
        case PrintIssueType::InconsistentNormals:
            return issueVisibility.normalProblems;
        case PrintIssueType::InvertedOrientation:
            return issueVisibility.invertedOrientation;
        case PrintIssueType::DisconnectedIsland:
            return issueVisibility.disconnectedIslands;
        case PrintIssueType::DegenerateGeometry:
            return issueVisibility.degenerateGeometry;
        case PrintIssueType::SelfIntersection:
            return issueVisibility.selfIntersections;
        case PrintIssueType::MinimumFeatureSize:
            return issueVisibility.minimumFeatureSize;
        case PrintIssueType::ThinWall:
            return issueVisibility.thinWalls;
        case PrintIssueType::Overhang:
            return issueVisibility.overhangs;
        case PrintIssueType::SupportRequired:
            return issueVisibility.supportRequired;
        }

        return false;
    }

    bool ManufacturingDisplaySettings::is_severity_visible(
        kernel::manufacturing::IssueSeverity severity) const noexcept
    {
        return severity_rank(severity) >= severity_rank(minimumSeverity);
    }

    ManufacturingVisualStyle ManufacturingDisplaySettings::style_for(
        kernel::manufacturing::PrintIssueType type) const noexcept
    {
        using kernel::manufacturing::PrintIssueType;

        switch (type) {
        case PrintIssueType::LooseEdge:
            return make_style({ 0.95f, 0.18f, 0.18f, 1.0f }, 90, 0.22f, 3.2f, 6.0f);
        case PrintIssueType::OpenBoundary:
            return make_style({ 1.0f, 0.06f, 0.08f, 1.0f }, 100, 0.26f, 5.0f, 7.5f);
        case PrintIssueType::NonManifoldEdge:
            return make_style({ 0.92f, 0.04f, 0.72f, 1.0f }, 110, 0.30f, 5.4f, 8.0f);
        case PrintIssueType::InconsistentNormals:
            return make_style({ 0.26f, 0.54f, 1.0f, 1.0f }, 70, 0.32f, 4.0f, 7.0f);
        case PrintIssueType::InvertedOrientation:
            return make_style({ 0.50f, 0.18f, 1.0f, 1.0f }, 75, 0.38f, 3.4f, 7.0f);
        case PrintIssueType::DisconnectedIsland:
            return make_style({ 0.15f, 0.85f, 0.70f, 1.0f }, 45, 0.30f, 3.0f, 6.5f);
        case PrintIssueType::DegenerateGeometry:
            return make_style({ 1.0f, 0.72f, 0.05f, 1.0f }, 120, 0.34f, 5.0f, 8.5f);
        case PrintIssueType::SelfIntersection:
            return make_style({ 1.0f, 0.28f, 0.02f, 1.0f }, 115, 0.40f, 4.8f, 8.0f);
        case PrintIssueType::MinimumFeatureSize:
            return make_style({ 0.98f, 0.86f, 0.12f, 1.0f }, 80, 0.34f, 4.2f, 7.0f);
        case PrintIssueType::ThinWall:
            return make_style({ 1.0f, 0.92f, 0.10f, 1.0f }, 85, 0.38f, 4.0f, 7.2f);
        case PrintIssueType::Overhang:
            return make_style({ 1.0f, 0.46f, 0.08f, 1.0f }, 60, 0.36f, 3.6f, 6.8f);
        case PrintIssueType::SupportRequired:
            return make_style({ 0.65f, 0.65f, 0.70f, 1.0f }, 35, 0.28f, 3.0f, 6.0f);
        }

        return {};
    }

} // namespace locus::editor
