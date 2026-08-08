/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/manufacturing/core/AnalysisContext.h"
#include "kernel/manufacturing/core/AnalysisReport.h"
#include "kernel/manufacturing/core/IAnalyzer.h"
#include "kernel/manufacturing/core/IssueLocation.h"
#include "kernel/manufacturing/core/IssueMeasurement.h"
#include "kernel/manufacturing/core/IssueSeverity.h"
#include "kernel/manufacturing/core/PrintIssue.h"
#include "kernel/manufacturing/core/PrintIssueType.h"
#include "kernel/manufacturing/profiles/PrintProfile.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Detects explicit editable features smaller than the minimum size
     * supported by the active manufacturing profile.
     *
     * The analyzer currently defines explicit feature size through LEM edge
     * length. Derived triangulation diagonals are intentionally ignored because
     * they are implementation details rather than authored geometric features.
     *
     * Connected undersized edges are grouped into one semantic issue so a
     * complete small feature can later be highlighted as a single region.
     *
     * Effectively zero-length edges are ignored here and remain the
     * responsibility of DegenerateGeometryAnalyzer. Wall thickness between
     * opposing surfaces remains the responsibility of thin-wall analysis.
     */
    class MinimumFeatureSizeAnalyzer final : public IAnalyzer {
    public:
        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "MinimumFeatureSizeAnalyzer";
        }

        /**
         * @brief Detects connected groups of undersized editable edges.
         *
         * Both a source LEM and an active profile with a positive finite
         * minimumFeatureSize limit are required.
         *
         * Existing issues and metrics are preserved.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving minimum-feature-size issues.
         */
        void analyze(
            const AnalysisContext& context,
            AnalysisReport& report) const override
        {
            if (!context.has_mesh() ||
                !context.has_profile()) {
                return;
            }

            const double minimumFeatureSize =
                configured_limit(*context.profile);

            if (!std::isfinite(minimumFeatureSize) ||
                minimumFeatureSize <= 0.0) {
                return;
            }

            const geometry::LEM& mesh =
                *context.mesh;

            const std::vector<UndersizedEdge> undersizedEdges =
                collect_undersized_edges(
                    mesh,
                    minimumFeatureSize);

            if (undersizedEdges.empty()) {
                return;
            }

            std::vector<geometry::EdgeHandle> visited;
            visited.reserve(undersizedEdges.size());

            for (const UndersizedEdge& edge :
                undersizedEdges) {

                if (contains(
                    visited,
                    edge.handle)) {
                    continue;
                }

                const std::vector<UndersizedEdge> component =
                    collect_component(
                        mesh,
                        edge.handle,
                        undersizedEdges,
                        visited);

                if (!component.empty()) {
                    report_feature(
                        mesh,
                        component,
                        minimumFeatureSize,
                        report);
                }
            }
        }

    private:
        /**
         * @brief Length below which geometry is considered degenerate rather
         * than merely small.
         */
        static constexpr double DegenerateLengthEpsilon = 1.0e-9;

        /**
         * @brief Cached length of an edge already classified as undersized.
         */
        struct UndersizedEdge {
            geometry::EdgeHandle handle{};
            double length = 0.0;
        };

        /**
         * @brief Returns the configured process-dependent feature-size limit.
         *
         * @param profile Active print profile.
         * @return Minimum feature size, or NaN when no limit is configured.
         */
        [[nodiscard]] static double configured_limit(
            const PrintProfile& profile)
        {
            const auto& limit =
                profile.limits().minimumFeatureSize;

            if (!limit.has_value()) {
                return
                    std::numeric_limits<double>::quiet_NaN();
            }

            return limit.value();
        }

        /**
         * @brief Finds explicit edges shorter than the process limit.
         *
         * Effectively zero-length edges are excluded so degeneracy is not
         * reported twice by separate analyzers.
         *
         * @param mesh Source editable mesh.
         * @param minimumFeatureSize Active process limit.
         * @return Undersized non-degenerate edges.
         */
        [[nodiscard]] static std::vector<UndersizedEdge>
            collect_undersized_edges(
                const geometry::LEM& mesh,
                double minimumFeatureSize)
        {
            std::vector<UndersizedEdge> result;

            for (const geometry::EdgeHandle edgeHandle :
            geometry::TopologyTraversal::edges(mesh)) {

                if (!mesh.is_valid(edgeHandle)) {
                    continue;
                }

                const auto endpoints =
                    geometry::TopologyTraversal::edge_vertices(
                        mesh,
                        edgeHandle);

                if (!mesh.is_valid(endpoints[0]) ||
                    !mesh.is_valid(endpoints[1])) {
                    continue;
                }

                const glm::vec3& a =
                    mesh.vertex(endpoints[0]).position;

                const glm::vec3& b =
                    mesh.vertex(endpoints[1]).position;

                const double length =
                    static_cast<double>(
                        glm::length(b - a));

                if (!std::isfinite(length) ||
                    length <= DegenerateLengthEpsilon ||
                    length >= minimumFeatureSize) {
                    continue;
                }

                result.push_back(
                    UndersizedEdge{
                        edgeHandle,
                        length
                    });
            }

            return result;
        }

        /**
         * @brief Collects one connected group of undersized edges.
         *
         * Connectivity is determined by shared editable vertices.
         *
         * @param mesh Source editable mesh.
         * @param startEdge First undersized edge in the component.
         * @param undersizedEdges Complete undersized-edge collection.
         * @param visited Edges already assigned to components.
         * @return Connected undersized feature.
         */
        [[nodiscard]] static std::vector<UndersizedEdge>
            collect_component(
                const geometry::LEM& mesh,
                geometry::EdgeHandle startEdge,
                const std::vector<UndersizedEdge>& undersizedEdges,
                std::vector<geometry::EdgeHandle>& visited)
        {
            std::vector<UndersizedEdge> component;
            std::vector<geometry::EdgeHandle> pending;

            pending.push_back(startEdge);

            while (!pending.empty()) {
                const geometry::EdgeHandle current =
                    pending.back();

                pending.pop_back();

                if (contains(visited, current)) {
                    continue;
                }

                const UndersizedEdge* currentEdge =
                    find_edge(
                        undersizedEdges,
                        current);

                if (currentEdge == nullptr ||
                    !mesh.is_valid(current)) {
                    continue;
                }

                visited.push_back(current);
                component.push_back(*currentEdge);

                const auto endpoints =
                    geometry::TopologyTraversal::edge_vertices(
                        mesh,
                        current);

                for (const UndersizedEdge& candidate :
                    undersizedEdges) {

                    if (contains(
                        visited,
                        candidate.handle) ||
                        candidate.handle == current) {
                        continue;
                    }

                    if (edge_uses_vertex(
                        mesh,
                        candidate.handle,
                        endpoints[0]) ||
                        edge_uses_vertex(
                            mesh,
                            candidate.handle,
                            endpoints[1])) {

                        pending.push_back(
                            candidate.handle);
                    }
                }
            }

            return component;
        }

        /**
         * @brief Creates one issue for a connected undersized feature.
         *
         * All involved edges, unique vertices, adjacent faces and their spatial
         * region are retained for future editor visualization.
         *
         * The issue measurement stores the smallest edge found in the feature,
         * compared with the active process limit.
         *
         * @param mesh Source editable mesh.
         * @param component Connected undersized edges.
         * @param minimumFeatureSize Active process limit.
         * @param report Report receiving the issue.
         */
        static void report_feature(
            const geometry::LEM& mesh,
            const std::vector<UndersizedEdge>& component,
            double minimumFeatureSize,
            AnalysisReport& report)
        {
            IssueLocation location;

            double smallestLength =
                std::numeric_limits<double>::infinity();

            for (const UndersizedEdge& edge :
                component) {

                if (!mesh.is_valid(edge.handle)) {
                    continue;
                }

                add_unique(
                    location.edges,
                    edge.handle);

                smallestLength =
                    std::min(
                        smallestLength,
                        edge.length);

                const auto endpoints =
                    geometry::TopologyTraversal::edge_vertices(
                        mesh,
                        edge.handle);

                for (const geometry::VertexHandle vertexHandle :
                endpoints) {

                    if (!mesh.is_valid(vertexHandle)) {
                        continue;
                    }

                    if (!contains(
                        location.vertices,
                        vertexHandle)) {

                        location.vertices.push_back(
                            vertexHandle);

                        const glm::vec3& position =
                            mesh.vertex(
                                vertexHandle).position;

                        location.samples.push_back(
                            position);

                        location.region.expand(
                            position);
                    }
                }

                const std::vector<geometry::FaceHandle> adjacentFaces =
                    geometry::TopologyTraversal::edge_faces(
                        mesh,
                        edge.handle);

                for (const geometry::FaceHandle faceHandle :
                adjacentFaces) {

                    if (mesh.is_valid(faceHandle)) {
                        add_unique(
                            location.faces,
                            faceHandle);
                    }
                }
            }

            if (!std::isfinite(smallestLength)) {
                return;
            }

            PrintIssue issue{
                PrintIssueType::MinimumFeatureSize,
                IssueSeverity::Warning,
                "Editable feature is smaller than the configured manufacturing limit.",
                std::move(location)
            };

            issue.measurement =
                IssueMeasurement{
                    IssueMeasurementKind::Length,
                    smallestLength,
                    minimumFeatureSize
            };

            report.add_issue(
                std::move(issue));
        }

        /**
         * @brief Finds cached data for an undersized edge.
         */
        [[nodiscard]] static const UndersizedEdge* find_edge(
            const std::vector<UndersizedEdge>& edges,
            geometry::EdgeHandle handle)
        {
            for (const UndersizedEdge& edge : edges) {
                if (edge.handle == handle) {
                    return &edge;
                }
            }

            return nullptr;
        }

        /**
         * @brief Checks whether an edge uses an editable vertex.
         */
        [[nodiscard]] static bool edge_uses_vertex(
            const geometry::LEM& mesh,
            geometry::EdgeHandle edgeHandle,
            geometry::VertexHandle vertexHandle)
        {
            if (!mesh.is_valid(edgeHandle) ||
                !mesh.is_valid(vertexHandle)) {
                return false;
            }

            const auto endpoints =
                geometry::TopologyTraversal::edge_vertices(
                    mesh,
                    edgeHandle);

            return
                endpoints[0] == vertexHandle ||
                endpoints[1] == vertexHandle;
        }

        /**
         * @brief Checks whether a handle occurs in a collection.
         */
        template <typename Handle>
        [[nodiscard]] static bool contains(
            const std::vector<Handle>& handles,
            Handle handle)
        {
            return
                std::find(
                    handles.begin(),
                    handles.end(),
                    handle) !=
                handles.end();
        }

        /**
         * @brief Adds a handle when not already present.
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