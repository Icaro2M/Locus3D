/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/manufacturing/core/AnalysisContext.h"
#include "kernel/manufacturing/core/AnalysisReport.h"
#include "kernel/manufacturing/core/IAnalyzer.h"
#include "kernel/manufacturing/core/IssueLocation.h"
#include "kernel/manufacturing/core/IssueMeasurement.h"
#include "kernel/manufacturing/core/IssueSeverity.h"
#include "kernel/manufacturing/core/PrintIssue.h"
#include "kernel/manufacturing/core/PrintIssueType.h"

#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Detects edge topology that is unsuitable for a manifold printable
     * surface.
     *
     * The analyzer reports:
     *
     * - loose edges, which participate in no face;
     * - non-manifold edges, which participate in more than two radial loops.
     *
     * Boundary edges with exactly one radial loop are intentionally left to
     * WatertightAnalyzer. This prevents duplicate diagnostics and allows that
     * analyzer to group connected boundary edges into meaningful open
     * contours.
     */
    class ManifoldAnalyzer final : public IAnalyzer {
    public:
        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "ManifoldAnalyzer";
        }

        /**
         * @brief Inspects the radial use count of every active LEM edge.
         *
         * Existing report contents are preserved. When no source LEM is
         * available, the analyzer performs no work.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving discovered topology issues.
         */
        void analyze(
            const AnalysisContext& context,
            AnalysisReport& report) const override
        {
            if (!context.has_mesh()) {
                return;
            }

            const geometry::LEM& mesh =
                *context.mesh;

            for (const geometry::EdgeHandle edgeHandle :
            geometry::TopologyTraversal::edges(mesh)) {

                analyze_edge(
                    mesh,
                    edgeHandle,
                    report);
            }
        }

    private:
        /**
         * @brief Classifies one active edge by radial-loop count.
         *
         * @param mesh Source editable mesh.
         * @param edgeHandle Edge to inspect.
         * @param report Report receiving a discovered issue.
         */
        static void analyze_edge(
            const geometry::LEM& mesh,
            geometry::EdgeHandle edgeHandle,
            AnalysisReport& report)
        {
            if (!mesh.is_valid(edgeHandle)) {
                return;
            }

            const std::vector<geometry::LoopHandle> radialLoops =
                geometry::TopologyTraversal::edge_loops(
                    mesh,
                    edgeHandle);

            if (radialLoops.empty()) {
                report_loose_edge(
                    mesh,
                    edgeHandle,
                    report);

                return;
            }

            if (radialLoops.size() > 2) {
                report_non_manifold_edge(
                    mesh,
                    edgeHandle,
                    radialLoops.size(),
                    report);
            }

            // Exactly one radial loop is a boundary condition and belongs to
            // WatertightAnalyzer.
            //
            // Exactly two radial loops is the normal manifold case.
        }

        /**
         * @brief Reports an edge that participates in no face.
         *
         * Endpoint vertices are included in IssueLocation so editor overlays
         * can highlight both the loose edge and its endpoints if desired.
         *
         * @param mesh Source editable mesh.
         * @param edgeHandle Loose edge.
         * @param report Report receiving the issue.
         */
        static void report_loose_edge(
            const geometry::LEM& mesh,
            geometry::EdgeHandle edgeHandle,
            AnalysisReport& report)
        {
            IssueLocation location;
            location.edges.push_back(edgeHandle);

            const auto endpoints =
                geometry::TopologyTraversal::edge_vertices(
                    mesh,
                    edgeHandle);

            for (const geometry::VertexHandle vertexHandle :
            endpoints) {

                if (mesh.is_valid(vertexHandle)) {
                    location.vertices.push_back(vertexHandle);
                }
            }

            PrintIssue issue{
                PrintIssueType::LooseEdge,
                IssueSeverity::Warning,
                "Loose edge does not belong to any face.",
                std::move(location)
            };

            issue.measurement = IssueMeasurement{
                IssueMeasurementKind::Count,
                0.0,
                2.0
            };

            report.add_issue(std::move(issue));
        }

        /**
         * @brief Reports an edge used by more than two radial loops.
         *
         * The edge and every valid adjacent source face are stored in the
         * issue location so the problem can later be visualized directly in
         * the editor.
         *
         * @param mesh Source editable mesh.
         * @param edgeHandle Non-manifold edge.
         * @param radialLoopCount Number of radial loops using the edge.
         * @param report Report receiving the issue.
         */
        static void report_non_manifold_edge(
            const geometry::LEM& mesh,
            geometry::EdgeHandle edgeHandle,
            std::size_t radialLoopCount,
            AnalysisReport& report)
        {
            IssueLocation location;
            location.edges.push_back(edgeHandle);

            const auto endpoints =
                geometry::TopologyTraversal::edge_vertices(
                    mesh,
                    edgeHandle);

            for (const geometry::VertexHandle vertexHandle :
            endpoints) {

                if (mesh.is_valid(vertexHandle)) {
                    location.vertices.push_back(vertexHandle);
                }
            }

            const std::vector<geometry::FaceHandle> adjacentFaces =
                geometry::TopologyTraversal::edge_faces(
                    mesh,
                    edgeHandle);

            for (const geometry::FaceHandle faceHandle :
            adjacentFaces) {

                if (mesh.is_valid(faceHandle)) {
                    location.faces.push_back(faceHandle);
                }
            }

            PrintIssue issue{
                PrintIssueType::NonManifoldEdge,
                IssueSeverity::Error,
                "Edge is shared by more than two surface loops.",
                std::move(location)
            };

            issue.measurement = IssueMeasurement{
                IssueMeasurementKind::Count,
                static_cast<double>(radialLoopCount),
                2.0
            };

            report.add_issue(std::move(issue));
        }
    };

}