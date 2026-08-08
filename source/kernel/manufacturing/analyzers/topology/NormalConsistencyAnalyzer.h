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
#include "kernel/manufacturing/core/IssueSeverity.h"
#include "kernel/manufacturing/core/PrintIssue.h"
#include "kernel/manufacturing/core/PrintIssueType.h"

#include <array>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Detects local winding inconsistencies between adjacent manifold
     * faces.
     *
     * For a consistently oriented polygonal surface, two faces sharing a
     * manifold edge must traverse that edge in opposite directions.
     *
     * Boundary edges and non-manifold edges are intentionally ignored because
     * they are diagnosed by WatertightAnalyzer and ManifoldAnalyzer.
     */
    class NormalConsistencyAnalyzer final : public IAnalyzer {
    public:
        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "NormalConsistencyAnalyzer";
        }

        /**
         * @brief Checks winding consistency across every manifold edge.
         *
         * Existing report contents are preserved. The analyzer performs no
         * work when no source LEM is available.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving winding-consistency issues.
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
         * @brief Checks orientation agreement across one manifold edge.
         *
         * @param mesh Source editable mesh.
         * @param edgeHandle Edge to inspect.
         * @param report Report receiving an issue when inconsistent.
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

            // Only ordinary manifold adjacency has an unambiguous
            // two-face winding relationship.
            if (radialLoops.size() != 2) {
                return;
            }

            const geometry::LoopHandle firstLoopHandle =
                radialLoops[0];

            const geometry::LoopHandle secondLoopHandle =
                radialLoops[1];

            if (!mesh.is_valid(firstLoopHandle) ||
                !mesh.is_valid(secondLoopHandle)) {
                return;
            }

            const geometry::Loop& firstLoop =
                mesh.loop(firstLoopHandle);

            const geometry::Loop& secondLoop =
                mesh.loop(secondLoopHandle);

            const DirectedEdge firstDirection =
                directed_edge(
                    mesh,
                    firstLoop);

            const DirectedEdge secondDirection =
                directed_edge(
                    mesh,
                    secondLoop);

            if (!firstDirection.valid ||
                !secondDirection.valid) {
                return;
            }

            if (!same_direction(
                firstDirection,
                secondDirection)) {
                return;
            }

            report_inconsistency(
                mesh,
                edgeHandle,
                firstLoop,
                secondLoop,
                report);
        }

        /**
         * @brief Directed use of an edge by one face loop.
         */
        struct DirectedEdge {
            geometry::VertexHandle from{};
            geometry::VertexHandle to{};
            bool valid = false;
        };

        /**
         * @brief Extracts the directed edge represented by a face loop.
         *
         * A loop uses its own vertex as the start and the next loop's vertex
         * as the destination.
         *
         * @param mesh Source editable mesh.
         * @param loop Face loop to inspect.
         * @return Directed edge use.
         */
        [[nodiscard]] static DirectedEdge directed_edge(
            const geometry::LEM& mesh,
            const geometry::Loop& loop)
        {
            if (!mesh.is_valid(loop.vertex) ||
                !mesh.is_valid(loop.next)) {
                return {};
            }

            const geometry::Loop& nextLoop =
                mesh.loop(loop.next);

            if (!mesh.is_valid(nextLoop.vertex)) {
                return {};
            }

            DirectedEdge result;
            result.from = loop.vertex;
            result.to = nextLoop.vertex;
            result.valid = true;

            return result;
        }

        /**
         * @brief Checks whether two face loops traverse their shared edge in
         * the same direction.
         *
         * @param first First directed use.
         * @param second Second directed use.
         * @return True when both uses have identical direction.
         */
        [[nodiscard]] static bool same_direction(
            const DirectedEdge& first,
            const DirectedEdge& second) noexcept
        {
            return first.from == second.from &&
                first.to == second.to;
        }

        /**
         * @brief Reports one local winding inconsistency.
         *
         * The shared edge, its endpoints and both adjacent faces are retained
         * so editor presentation can highlight the exact conflicting region.
         *
         * @param mesh Source editable mesh.
         * @param edgeHandle Shared conflicting edge.
         * @param firstLoop First radial loop.
         * @param secondLoop Second radial loop.
         * @param report Report receiving the issue.
         */
        static void report_inconsistency(
            const geometry::LEM& mesh,
            geometry::EdgeHandle edgeHandle,
            const geometry::Loop& firstLoop,
            const geometry::Loop& secondLoop,
            AnalysisReport& report)
        {
            IssueLocation location;

            location.edges.push_back(edgeHandle);

            const std::array<geometry::VertexHandle, 2> endpoints =
                geometry::TopologyTraversal::edge_vertices(
                    mesh,
                    edgeHandle);

            for (const geometry::VertexHandle vertexHandle :
            endpoints) {

                if (mesh.is_valid(vertexHandle)) {
                    location.vertices.push_back(vertexHandle);
                }
            }

            if (mesh.is_valid(firstLoop.face)) {
                location.faces.push_back(firstLoop.face);
            }

            if (mesh.is_valid(secondLoop.face) &&
                secondLoop.face != firstLoop.face) {

                location.faces.push_back(secondLoop.face);
            }

            PrintIssue issue{
                PrintIssueType::InconsistentNormals,
                IssueSeverity::Error,
                "Adjacent faces traverse their shared edge in the same direction.",
                std::move(location)
            };

            report.add_issue(std::move(issue));
        }
    };

}