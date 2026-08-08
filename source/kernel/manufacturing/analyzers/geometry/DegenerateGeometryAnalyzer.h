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
     * @brief Detects geometry with effectively zero spatial extent.
     *
     * This analyzer is concerned with manufacturing-relevant geometric
     * degeneracy rather than structural LEM validity. It reports:
     *
     * - editable edges whose endpoints occupy effectively the same position;
     * - editable faces whose complete canonical triangulation has effectively
     *   zero total area.
     *
     * Small but geometrically valid features are intentionally not reported
     * here. Process-dependent size constraints belong to
     * MinimumFeatureSizeAnalyzer.
     */
    class DegenerateGeometryAnalyzer final : public IAnalyzer {
    public:
        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "DegenerateGeometryAnalyzer";
        }

        /**
         * @brief Detects degenerate editable edges and surface faces.
         *
         * Edge analysis requires only the authoritative LEM. Face-area
         * analysis additionally uses AnalysisMesh when available.
         *
         * Existing report contents are preserved.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving degeneracy findings.
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

            analyze_edges(
                mesh,
                report);

            if (context.has_analysis_mesh()) {
                analyze_faces(
                    mesh,
                    *context.analysisMesh,
                    report);
            }
        }

    private:
        /**
         * @brief Length tolerance used exclusively to classify effectively
         * zero-length geometry.
         *
         * This value is intentionally independent from print-profile limits.
         */
        static constexpr double LengthEpsilon = 1.0e-9;

        /**
         * @brief Area tolerance used exclusively to classify effectively
         * zero-area surfaces.
         */
        static constexpr double AreaEpsilon = 1.0e-18;

        /**
         * @brief Detects effectively zero-length editable edges.
         *
         * @param mesh Source editable mesh.
         * @param report Report receiving edge degeneracy findings.
         */
        static void analyze_edges(
            const geometry::LEM& mesh,
            AnalysisReport& report)
        {
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
                    length > LengthEpsilon) {
                    continue;
                }

                IssueLocation location;

                location.edges.push_back(
                    edgeHandle);

                location.vertices.push_back(
                    endpoints[0]);

                if (endpoints[1] != endpoints[0]) {
                    location.vertices.push_back(
                        endpoints[1]);
                }

                location.samples.push_back(a);
                location.samples.push_back(b);

                PrintIssue issue{
                    PrintIssueType::DegenerateGeometry,
                    IssueSeverity::Error,
                    "Edge has effectively zero geometric length.",
                    std::move(location)
                };

                issue.measurement =
                    IssueMeasurement{
                        IssueMeasurementKind::Length,
                        length,
                        LengthEpsilon
                };

                report.add_issue(
                    std::move(issue));
            }
        }

        /**
         * @brief Detects editable faces whose canonical analysis triangles
         * have effectively zero total area.
         *
         * A face is evaluated as one semantic entity even when triangulation
         * produces several triangles. This avoids emitting several issues for
         * one degenerate n-gon.
         *
         * @param mesh Source editable mesh.
         * @param analysisMesh Canonical manufacturing triangulation.
         * @param report Report receiving face degeneracy findings.
         */
        static void analyze_faces(
            const geometry::LEM& mesh,
            const AnalysisMesh& analysisMesh,
            AnalysisReport& report)
        {
            for (const geometry::FaceHandle faceHandle :
            geometry::TopologyTraversal::faces(mesh)) {

                if (!mesh.is_valid(faceHandle)) {
                    continue;
                }

                double totalArea = 0.0;
                bool hasTriangle = false;

                IssueLocation location;
                location.faces.push_back(
                    faceHandle);

                const std::vector<geometry::VertexHandle> faceVertices =
                    geometry::TopologyTraversal::face_vertices(
                        mesh,
                        faceHandle);

                for (const geometry::VertexHandle vertexHandle :
                faceVertices) {

                    if (!mesh.is_valid(vertexHandle)) {
                        continue;
                    }

                    location.vertices.push_back(
                        vertexHandle);

                    location.samples.push_back(
                        mesh.vertex(vertexHandle).position);
                }

                const std::vector<geometry::EdgeHandle> faceEdges =
                    geometry::TopologyTraversal::face_edges(
                        mesh,
                        faceHandle);

                for (const geometry::EdgeHandle edgeHandle :
                faceEdges) {

                    if (mesh.is_valid(edgeHandle)) {
                        location.edges.push_back(
                            edgeHandle);
                    }
                }

                for (std::size_t triangleIndex = 0;
                    triangleIndex < analysisMesh.triangle_count();
                    ++triangleIndex) {

                    if (analysisMesh.mapping()
                        .face_for_triangle(triangleIndex) !=
                        faceHandle) {
                        continue;
                    }

                    const AnalysisTriangle& triangle =
                        analysisMesh.triangle(
                            static_cast<AnalysisIndex>(
                                triangleIndex));

                    if (triangle.a >= analysisMesh.vertex_count() ||
                        triangle.b >= analysisMesh.vertex_count() ||
                        triangle.c >= analysisMesh.vertex_count()) {
                        continue;
                    }

                    const glm::vec3& a =
                        analysisMesh.vertex(
                            triangle.a).position;

                    const glm::vec3& b =
                        analysisMesh.vertex(
                            triangle.b).position;

                    const glm::vec3& c =
                        analysisMesh.vertex(
                            triangle.c).position;

                    const double area =
                        triangle_area(
                            a,
                            b,
                            c);

                    if (!std::isfinite(area)) {
                        continue;
                    }

                    totalArea += area;
                    hasTriangle = true;
                }

                /*
                 * A structurally present face that yielded no usable canonical
                 * triangles is also degenerate from the point of view of
                 * manufacturing analysis.
                 */
                if (hasTriangle &&
                    totalArea > AreaEpsilon) {
                    continue;
                }

                PrintIssue issue{
                    PrintIssueType::DegenerateGeometry,
                    IssueSeverity::Error,
                    hasTriangle
                        ? "Face has effectively zero geometric area."
                        : "Face produced no usable analysis triangles.",
                    std::move(location)
                };

                if (hasTriangle) {
                    issue.measurement =
                        IssueMeasurement{
                            IssueMeasurementKind::Area,
                            totalArea,
                            AreaEpsilon
                    };
                }

                report.add_issue(
                    std::move(issue));
            }
        }

        /**
         * @brief Computes the area of one triangle.
         *
         * @param a First position.
         * @param b Second position.
         * @param c Third position.
         * @return Triangle area.
         */
        [[nodiscard]] static double triangle_area(
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c) noexcept
        {
            const glm::dvec3 ad{ a };
            const glm::dvec3 bd{ b };
            const glm::dvec3 cd{ c };

            return
                0.5 *
                glm::length(
                    glm::cross(
                        bd - ad,
                        cd - ad));
        }
    };

}