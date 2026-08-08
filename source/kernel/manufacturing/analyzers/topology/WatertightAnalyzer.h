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

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Detects connected open boundary components in an editable mesh.
     *
     * An edge is considered an open boundary when it participates in exactly
     * one radial loop. Loose edges with no radial loops are intentionally
     * handled by ManifoldAnalyzer.
     *
     * Connected boundary edges are grouped into a single PrintIssue so a
     * complete opening can later be presented and highlighted as one semantic
     * manufacturing problem.
     */
    class WatertightAnalyzer final : public IAnalyzer {
    public:
        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "WatertightAnalyzer";
        }

        /**
         * @brief Finds and groups all open boundary components.
         *
         * Existing report contents are preserved. When no source mesh is
         * available, no analysis is performed.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving open-boundary issues.
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

            const std::vector<geometry::EdgeHandle> boundaryEdges =
                collect_boundary_edges(mesh);

            std::vector<geometry::EdgeHandle> visited;
            visited.reserve(boundaryEdges.size());

            for (const geometry::EdgeHandle edgeHandle :
            boundaryEdges) {

                if (contains(visited, edgeHandle)) {
                    continue;
                }

                std::vector<geometry::EdgeHandle> component =
                    collect_boundary_component(
                        mesh,
                        edgeHandle,
                        boundaryEdges,
                        visited);

                if (!component.empty()) {
                    report_boundary_component(
                        mesh,
                        component,
                        report);
                }
            }
        }

    private:
        /**
         * @brief Collects edges participating in exactly one radial loop.
         *
         * @param mesh Source editable mesh.
         * @return Open boundary edges.
         */
        [[nodiscard]] static std::vector<geometry::EdgeHandle>
            collect_boundary_edges(
                const geometry::LEM& mesh)
        {
            std::vector<geometry::EdgeHandle> result;

            for (const geometry::EdgeHandle edgeHandle :
            geometry::TopologyTraversal::edges(mesh)) {

                if (!mesh.is_valid(edgeHandle)) {
                    continue;
                }

                const std::vector<geometry::LoopHandle> radialLoops =
                    geometry::TopologyTraversal::edge_loops(
                        mesh,
                        edgeHandle);

                if (radialLoops.size() == 1) {
                    result.push_back(edgeHandle);
                }
            }

            return result;
        }

        /**
         * @brief Collects one connected component of boundary edges.
         *
         * Connectivity is defined by shared endpoint vertices. This supports
         * ordinary closed hole contours as well as malformed branching or open
         * boundary networks without assuming every boundary is a simple loop.
         *
         * @param mesh Source editable mesh.
         * @param startEdge First edge in the component.
         * @param boundaryEdges Complete boundary-edge set.
         * @param visited Boundary edges already assigned to components.
         * @return Connected boundary-edge component.
         */
        [[nodiscard]] static std::vector<geometry::EdgeHandle>
            collect_boundary_component(
                const geometry::LEM& mesh,
                geometry::EdgeHandle startEdge,
                const std::vector<geometry::EdgeHandle>& boundaryEdges,
                std::vector<geometry::EdgeHandle>& visited)
        {
            std::vector<geometry::EdgeHandle> component;
            std::vector<geometry::EdgeHandle> pending;

            pending.push_back(startEdge);

            while (!pending.empty()) {
                const geometry::EdgeHandle current =
                    pending.back();

                pending.pop_back();

                if (!mesh.is_valid(current) ||
                    contains(visited, current)) {
                    continue;
                }

                visited.push_back(current);
                component.push_back(current);

                const auto endpoints =
                    geometry::TopologyTraversal::edge_vertices(
                        mesh,
                        current);

                for (const geometry::VertexHandle vertexHandle :
                endpoints) {

                    if (!mesh.is_valid(vertexHandle)) {
                        continue;
                    }

                    for (const geometry::EdgeHandle candidate :
                    boundaryEdges) {

                        if (contains(visited, candidate) ||
                            !mesh.is_valid(candidate)) {
                            continue;
                        }

                        if (edge_uses_vertex(
                            mesh,
                            candidate,
                            vertexHandle)) {

                            pending.push_back(candidate);
                        }
                    }
                }
            }

            return component;
        }

        /**
         * @brief Reports one connected open boundary.
         *
         * All boundary edges, unique endpoint vertices, and unique adjacent
         * faces are preserved so editor presentation can highlight the entire
         * affected contour or network.
         *
         * @param mesh Source editable mesh.
         * @param component Boundary edges belonging to one connected component.
         * @param report Report receiving the issue.
         */
        static void report_boundary_component(
            const geometry::LEM& mesh,
            const std::vector<geometry::EdgeHandle>& component,
            AnalysisReport& report)
        {
            IssueLocation location;

            for (const geometry::EdgeHandle edgeHandle :
            component) {

                if (!mesh.is_valid(edgeHandle)) {
                    continue;
                }

                add_unique(
                    location.edges,
                    edgeHandle);

                const auto endpoints =
                    geometry::TopologyTraversal::edge_vertices(
                        mesh,
                        edgeHandle);

                for (const geometry::VertexHandle vertexHandle :
                endpoints) {

                    if (mesh.is_valid(vertexHandle)) {
                        add_unique(
                            location.vertices,
                            vertexHandle);
                    }
                }

                const std::vector<geometry::FaceHandle> adjacentFaces =
                    geometry::TopologyTraversal::edge_faces(
                        mesh,
                        edgeHandle);

                for (const geometry::FaceHandle faceHandle :
                adjacentFaces) {

                    if (mesh.is_valid(faceHandle)) {
                        add_unique(
                            location.faces,
                            faceHandle);
                    }
                }
            }

            PrintIssue issue{
                PrintIssueType::OpenBoundary,
                IssueSeverity::Error,
                "Mesh contains a connected open boundary.",
                std::move(location)
            };

            issue.measurement = IssueMeasurement{
                IssueMeasurementKind::Count,
                static_cast<double>(component.size()),
                0.0
            };

            report.add_issue(std::move(issue));
        }

        /**
         * @brief Checks whether an edge uses a specified endpoint vertex.
         *
         * @param mesh Source editable mesh.
         * @param edgeHandle Edge to inspect.
         * @param vertexHandle Vertex to find.
         * @return True when vertexHandle is one of the edge endpoints.
         */
        [[nodiscard]] static bool edge_uses_vertex(
            const geometry::LEM& mesh,
            geometry::EdgeHandle edgeHandle,
            geometry::VertexHandle vertexHandle)
        {
            const auto endpoints =
                geometry::TopologyTraversal::edge_vertices(
                    mesh,
                    edgeHandle);

            return endpoints[0] == vertexHandle ||
                endpoints[1] == vertexHandle;
        }

        /**
         * @brief Checks whether a handle already exists in a collection.
         *
         * @tparam Handle Handle type.
         * @param handles Collection to inspect.
         * @param handle Handle to find.
         * @return True when the handle already exists.
         */
        template <typename Handle>
        [[nodiscard]] static bool contains(
            const std::vector<Handle>& handles,
            Handle handle)
        {
            return std::find(
                handles.begin(),
                handles.end(),
                handle) != handles.end();
        }

        /**
         * @brief Adds a handle only when it is not already stored.
         *
         * @tparam Handle Handle type.
         * @param handles Collection receiving the handle.
         * @param handle Handle to add.
         */
        template <typename Handle>
        static void add_unique(
            std::vector<Handle>& handles,
            Handle handle)
        {
            if (!contains(handles, handle)) {
                handles.push_back(handle);
            }
        }
    };

}