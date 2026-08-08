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
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Detects disconnected surface components in an editable mesh.
     *
     * Faces belong to the same component when they are connected through
     * shared edges. When more than one surface component exists, the largest
     * component by face count is treated as the principal component and each
     * remaining component is reported as a disconnected island.
     *
     * Loose edges and isolated non-surface geometry are intentionally outside
     * this analyzer's responsibility and are handled by topology analyzers
     * such as ManifoldAnalyzer.
     */
    class IslandAnalyzer final : public IAnalyzer {
    public:
        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "IslandAnalyzer";
        }

        /**
         * @brief Finds disconnected surface components.
         *
         * The number of discovered face components is stored in
         * AnalysisMetrics::connectedComponentCount. A warning is generated
         * for every component other than the largest one.
         *
         * Existing report issues and unrelated metrics are preserved.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving component metrics and island issues.
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

            const std::vector<Component> components =
                collect_components(mesh);

            report.metrics().connectedComponentCount =
                components.size();

            if (components.size() <= 1) {
                return;
            }

            const std::size_t principalIndex =
                principal_component_index(components);

            for (std::size_t index = 0;
                index < components.size();
                ++index) {

                if (index == principalIndex) {
                    continue;
                }

                report_island(
                    mesh,
                    components[index],
                    report);
            }
        }

    private:
        /**
         * @brief Connected collection of editable surface faces.
         */
        struct Component {
            std::vector<geometry::FaceHandle> faces{};
        };

        /**
         * @brief Collects all edge-connected face components.
         *
         * @param mesh Source editable mesh.
         * @return Connected surface components.
         */
        [[nodiscard]] static std::vector<Component>
            collect_components(
                const geometry::LEM& mesh)
        {
            std::vector<Component> components;
            std::vector<geometry::FaceHandle> visited;

            const std::vector<geometry::FaceHandle> faces =
                geometry::TopologyTraversal::faces(mesh);

            visited.reserve(faces.size());

            for (const geometry::FaceHandle faceHandle : faces) {
                if (!mesh.is_valid(faceHandle) ||
                    contains(visited, faceHandle)) {
                    continue;
                }

                Component component;

                collect_component(
                    mesh,
                    faceHandle,
                    visited,
                    component);

                if (!component.faces.empty()) {
                    components.push_back(
                        std::move(component));
                }
            }

            return components;
        }

        /**
         * @brief Traverses one connected face component.
         *
         * @param mesh Source editable mesh.
         * @param startFace First face in the component.
         * @param visited Faces already assigned to components.
         * @param component Component receiving discovered faces.
         */
        static void collect_component(
            const geometry::LEM& mesh,
            geometry::FaceHandle startFace,
            std::vector<geometry::FaceHandle>& visited,
            Component& component)
        {
            std::vector<geometry::FaceHandle> pending;
            pending.push_back(startFace);

            while (!pending.empty()) {
                const geometry::FaceHandle current =
                    pending.back();

                pending.pop_back();

                if (!mesh.is_valid(current) ||
                    contains(visited, current)) {
                    continue;
                }

                visited.push_back(current);
                component.faces.push_back(current);

                const std::vector<geometry::EdgeHandle> faceEdges =
                    geometry::TopologyTraversal::face_edges(
                        mesh,
                        current);

                for (const geometry::EdgeHandle edgeHandle :
                faceEdges) {

                    if (!mesh.is_valid(edgeHandle)) {
                        continue;
                    }

                    const std::vector<geometry::FaceHandle> adjacentFaces =
                        geometry::TopologyTraversal::edge_faces(
                            mesh,
                            edgeHandle);

                    for (const geometry::FaceHandle adjacentFace :
                    adjacentFaces) {

                        if (mesh.is_valid(adjacentFace) &&
                            !contains(visited, adjacentFace)) {

                            pending.push_back(adjacentFace);
                        }
                    }
                }
            }
        }

        /**
         * @brief Selects the largest surface component.
         *
         * When components have equal face counts, the first discovered
         * component is chosen. This keeps selection deterministic.
         *
         * @param components Surface components.
         * @return Index of the principal component.
         */
        [[nodiscard]] static std::size_t principal_component_index(
            const std::vector<Component>& components)
        {
            std::size_t principalIndex = 0;
            std::size_t largestFaceCount = 0;

            for (std::size_t index = 0;
                index < components.size();
                ++index) {

                const std::size_t faceCount =
                    components[index].faces.size();

                if (faceCount > largestFaceCount) {
                    largestFaceCount = faceCount;
                    principalIndex = index;
                }
            }

            return principalIndex;
        }

        /**
         * @brief Reports one disconnected surface island.
         *
         * Faces, edges and vertices belonging to the island are retained so
         * editor presentation can highlight the complete disconnected region.
         *
         * @param mesh Source editable mesh.
         * @param component Disconnected component.
         * @param report Report receiving the issue.
         */
        static void report_island(
            const geometry::LEM& mesh,
            const Component& component,
            AnalysisReport& report)
        {
            IssueLocation location;

            for (const geometry::FaceHandle faceHandle :
            component.faces) {

                if (!mesh.is_valid(faceHandle)) {
                    continue;
                }

                add_unique(
                    location.faces,
                    faceHandle);

                const std::vector<geometry::EdgeHandle> edges =
                    geometry::TopologyTraversal::face_edges(
                        mesh,
                        faceHandle);

                for (const geometry::EdgeHandle edgeHandle :
                edges) {

                    if (mesh.is_valid(edgeHandle)) {
                        add_unique(
                            location.edges,
                            edgeHandle);
                    }
                }

                const std::vector<geometry::VertexHandle> vertices =
                    geometry::TopologyTraversal::face_vertices(
                        mesh,
                        faceHandle);

                for (const geometry::VertexHandle vertexHandle :
                vertices) {

                    if (mesh.is_valid(vertexHandle)) {
                        add_unique(
                            location.vertices,
                            vertexHandle);
                    }
                }
            }

            PrintIssue issue{
                PrintIssueType::DisconnectedIsland,
                IssueSeverity::Warning,
                "Mesh contains a disconnected surface island.",
                std::move(location)
            };

            issue.measurement = IssueMeasurement{
                IssueMeasurementKind::Count,
                static_cast<double>(
                    component.faces.size()),
                std::nullopt
            };

            report.add_issue(
                std::move(issue));
        }

        /**
         * @brief Checks whether a handle exists in a collection.
         *
         * @tparam Handle Handle type.
         * @param handles Collection to inspect.
         * @param handle Handle to find.
         * @return True when the handle is present.
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
         * @brief Adds a handle when not already present.
         *
         * @tparam Handle Handle type.
         * @param handles Collection receiving the handle.
         * @param handle Handle to append.
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