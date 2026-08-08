/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/manufacturing/core/AnalysisContext.h"
#include "kernel/manufacturing/core/AnalysisReport.h"
#include "kernel/manufacturing/core/IAnalyzer.h"
#include "kernel/manufacturing/mesh/AnalysisMesh.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <cmath>
#include <cstddef>
#include <string_view>

namespace locus::kernel::manufacturing {

    /**
     * @brief Computes enclosed volume for closed manufacturing geometry.
     *
     * Volume is calculated from the canonical triangulation stored in
     * AnalysisMesh using signed tetrahedron accumulation.
     *
     * The resulting AnalysisMetrics value is always non-negative. Global
     * surface orientation is diagnosed separately by OrientationAnalyzer.
     *
     * Open or non-manifold meshes do not receive a volume measurement because
     * an enclosed volume cannot be interpreted reliably in those conditions.
     */
    class VolumeAnalyzer final : public IAnalyzer {
    public:
        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "VolumeAnalyzer";
        }

        /**
         * @brief Computes enclosed volume when the source mesh is suitable.
         *
         * Existing issues and unrelated metrics are preserved.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving the volume metric.
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

            if (analysisMesh.empty() ||
                !is_closed_manifold(mesh)) {
                return;
            }

            double signedVolume = 0.0;
            std::size_t validTriangleCount = 0;

            for (std::size_t triangleIndex = 0;
                triangleIndex < analysisMesh.triangle_count();
                ++triangleIndex) {

                const AnalysisTriangle& triangle =
                    analysisMesh.triangle(
                        static_cast<AnalysisIndex>(
                            triangleIndex));

                if (!triangle_indices_valid(
                    analysisMesh,
                    triangle)) {
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

                signedVolume +=
                    signed_tetrahedron_volume(
                        a,
                        b,
                        c);

                ++validTriangleCount;
            }

            if (validTriangleCount == 0 ||
                !std::isfinite(signedVolume) ||
                std::abs(signedVolume) <= VolumeEpsilon) {
                return;
            }

            report.metrics().volume =
                std::abs(signedVolume);
        }

    private:
        /**
         * @brief Small signed-volume magnitude treated as indeterminate.
         */
        static constexpr double VolumeEpsilon = 1.0e-12;

        /**
         * @brief Checks whether every active source edge belongs to exactly
         * two radial loops.
         *
         * @param mesh Source editable mesh.
         * @return True when the mesh forms a closed manifold surface.
         */
        [[nodiscard]] static bool is_closed_manifold(
            const geometry::LEM& mesh)
        {
            const auto edges =
                geometry::TopologyTraversal::edges(mesh);

            if (edges.empty()) {
                return false;
            }

            for (const geometry::EdgeHandle edgeHandle :
            edges) {

                if (!mesh.is_valid(edgeHandle)) {
                    return false;
                }

                const auto radialLoops =
                    geometry::TopologyTraversal::edge_loops(
                        mesh,
                        edgeHandle);

                if (radialLoops.size() != 2) {
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief Checks whether all triangle vertex indices are available.
         *
         * @param mesh Analysis mesh containing the triangle.
         * @param triangle Triangle to inspect.
         * @return True when all referenced vertices exist.
         */
        [[nodiscard]] static bool triangle_indices_valid(
            const AnalysisMesh& mesh,
            const AnalysisTriangle& triangle) noexcept
        {
            return
                triangle.a < mesh.vertex_count() &&
                triangle.b < mesh.vertex_count() &&
                triangle.c < mesh.vertex_count();
        }

        /**
         * @brief Computes signed volume of the tetrahedron formed by a
         * triangle and the origin.
         *
         * Summing this quantity over a consistently wound closed surface
         * yields its signed enclosed volume.
         *
         * @param a First triangle position.
         * @param b Second triangle position.
         * @param c Third triangle position.
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
    };

}