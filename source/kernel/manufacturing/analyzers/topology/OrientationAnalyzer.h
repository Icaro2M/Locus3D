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
#include "kernel/manufacturing/mesh/AnalysisMesh.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Detects globally inverted closed surface components.
     *
     * Local winding agreement is handled separately by
     * NormalConsistencyAnalyzer. This analyzer addresses the case where an
     * entire closed component is internally consistent but oriented inward.
     *
     * Orientation is determined from signed volume computed over the canonical
     * AnalysisMesh triangulation.
     */
    class OrientationAnalyzer final : public IAnalyzer {
    public:
        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "OrientationAnalyzer";
        }

        /**
         * @brief Checks the global orientation of closed connected components.
         *
         * Both the authoritative LEM and its derived AnalysisMesh are required.
         * Open or non-manifold components are ignored because signed-volume
         * orientation is not considered reliable for them.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving inverted-orientation issues.
         */
        void analyze(
            const AnalysisContext& context,
            AnalysisReport& report) const override
        {
            if (!context.has_mesh() ||
                !context.has_analysis_mesh()) {
                return;
            }

            const geometry::LEM& mesh =
                *context.mesh;

            const AnalysisMesh& analysisMesh =
                *context.analysisMesh;

            std::vector<geometry::FaceHandle> visited;

            const std::vector<geometry::FaceHandle> faces =
                geometry::TopologyTraversal::faces(mesh);

            visited.reserve(faces.size());

            for (const geometry::FaceHandle faceHandle : faces) {
                if (!mesh.is_valid(faceHandle) ||
                    contains(visited, faceHandle)) {
                    continue;
                }

                const std::vector<geometry::FaceHandle> component =
                    collect_component(
                        mesh,
                        faceHandle,
                        visited);

                if (component.empty() ||
                    !is_closed_manifold_component(
                        mesh,
                        component)) {
                    continue;
                }

                analyze_component(
                    analysisMesh,
                    component,
                    report);
            }
        }

    private:
        /**
         * @brief Minimum absolute signed volume used to classify orientation.
         *
         * Values at or below this magnitude are treated as indeterminate.
         */
        static constexpr double VolumeEpsilon = 1.0e-12;

        /**
         * @brief Collects a connected set of faces.
         *
         * Face connectivity is established through shared edges.
         *
         * @param mesh Source editable mesh.
         * @param startFace First face in the component.
         * @param visited Faces already assigned to a component.
         * @return Connected face component.
         */
        [[nodiscard]] static std::vector<geometry::FaceHandle>
            collect_component(
                const geometry::LEM& mesh,
                geometry::FaceHandle startFace,
                std::vector<geometry::FaceHandle>& visited)
        {
            std::vector<geometry::FaceHandle> component;
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
                component.push_back(current);

                const std::vector<geometry::EdgeHandle> edges =
                    geometry::TopologyTraversal::face_edges(
                        mesh,
                        current);

                for (const geometry::EdgeHandle edgeHandle : edges) {
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

            return component;
        }

        /**
         * @brief Checks whether a connected face component forms a closed
         * manifold shell.
         *
         * Every edge used by every component face must participate in exactly
         * two radial loops, and both adjacent faces must belong to the same
         * component.
         *
         * @param mesh Source editable mesh.
         * @param component Connected face component.
         * @return True when the component forms a closed manifold surface.
         */
        [[nodiscard]] static bool is_closed_manifold_component(
            const geometry::LEM& mesh,
            const std::vector<geometry::FaceHandle>& component)
        {
            for (const geometry::FaceHandle faceHandle : component) {
                const std::vector<geometry::EdgeHandle> edges =
                    geometry::TopologyTraversal::face_edges(
                        mesh,
                        faceHandle);

                for (const geometry::EdgeHandle edgeHandle : edges) {
                    const std::vector<geometry::LoopHandle> radialLoops =
                        geometry::TopologyTraversal::edge_loops(
                            mesh,
                            edgeHandle);

                    if (radialLoops.size() != 2) {
                        return false;
                    }

                    const std::vector<geometry::FaceHandle> adjacentFaces =
                        geometry::TopologyTraversal::edge_faces(
                            mesh,
                            edgeHandle);

                    if (adjacentFaces.size() != 2) {
                        return false;
                    }

                    for (const geometry::FaceHandle adjacentFace :
                    adjacentFaces) {

                        if (!contains(component, adjacentFace)) {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        /**
         * @brief Computes and classifies one closed component.
         *
         * @param analysisMesh Canonical triangulated analysis representation.
         * @param component Source LEM faces belonging to the component.
         * @param report Report receiving an issue when orientation is inward.
         */
        static void analyze_component(
            const AnalysisMesh& analysisMesh,
            const std::vector<geometry::FaceHandle>& component,
            AnalysisReport& report)
        {
            double signedVolume = 0.0;
            std::size_t usedTriangleCount = 0;

            for (std::size_t triangleIndex = 0;
                triangleIndex < analysisMesh.triangle_count();
                ++triangleIndex) {

                const geometry::FaceHandle sourceFace =
                    analysisMesh.mapping()
                    .face_for_triangle(triangleIndex);

                if (!contains(component, sourceFace)) {
                    continue;
                }

                const AnalysisTriangle& triangle =
                    analysisMesh.triangle(
                        static_cast<AnalysisIndex>(triangleIndex));

                if (triangle.a >= analysisMesh.vertex_count() ||
                    triangle.b >= analysisMesh.vertex_count() ||
                    triangle.c >= analysisMesh.vertex_count()) {
                    continue;
                }

                const glm::vec3& a =
                    analysisMesh.vertex(triangle.a).position;

                const glm::vec3& b =
                    analysisMesh.vertex(triangle.b).position;

                const glm::vec3& c =
                    analysisMesh.vertex(triangle.c).position;

                signedVolume +=
                    signed_tetrahedron_volume(
                        a,
                        b,
                        c);

                ++usedTriangleCount;
            }

            if (usedTriangleCount == 0 ||
                std::abs(signedVolume) <= VolumeEpsilon) {
                return;
            }

            if (signedVolume > 0.0) {
                return;
            }

            report_inverted_component(
                component,
                signedVolume,
                report);
        }

        /**
         * @brief Computes the signed volume of a triangle with the origin as
         * the fourth tetrahedron point.
         *
         * Summing this expression over a closed consistently wound surface
         * yields the signed enclosed volume.
         *
         * @param a First triangle vertex.
         * @param b Second triangle vertex.
         * @param c Third triangle vertex.
         * @return Signed tetrahedron volume.
         */
        [[nodiscard]] static double signed_tetrahedron_volume(
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c) noexcept
        {
            const glm::dvec3 ad{ a };
            const glm::dvec3 bd{ b };
            const glm::dvec3 cd{ c };

            return
                glm::dot(
                    ad,
                    glm::cross(bd, cd)) /
                6.0;
        }

        /**
         * @brief Reports a consistently inward-oriented closed component.
         *
         * Every face in the component is included so future editor
         * presentation can highlight the complete inverted shell.
         *
         * @param component Faces belonging to the inverted shell.
         * @param signedVolume Negative signed volume measured for the component.
         * @param report Report receiving the issue.
         */
        static void report_inverted_component(
            const std::vector<geometry::FaceHandle>& component,
            double signedVolume,
            AnalysisReport& report)
        {
            IssueLocation location;
            location.faces = component;

            PrintIssue issue{
                PrintIssueType::InvertedOrientation,
                IssueSeverity::Error,
                "Closed surface component is consistently oriented inward.",
                std::move(location)
            };

            issue.measurement = IssueMeasurement{
                IssueMeasurementKind::Volume,
                signedVolume,
                0.0
            };

            report.add_issue(std::move(issue));
        }

        /**
         * @brief Checks whether a handle occurs in a collection.
         *
         * @tparam Handle Handle type.
         * @param handles Collection to inspect.
         * @param handle Handle to find.
         * @return True when handle is present.
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
    };

}