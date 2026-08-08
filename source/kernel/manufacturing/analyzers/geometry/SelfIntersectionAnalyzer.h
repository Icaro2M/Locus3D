/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/spatial/BVHQuery.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/manufacturing/core/AnalysisContext.h"
#include "kernel/manufacturing/core/AnalysisReport.h"
#include "kernel/manufacturing/core/IAnalyzer.h"
#include "kernel/manufacturing/core/IssueLocation.h"
#include "kernel/manufacturing/core/IssueSeverity.h"
#include "kernel/manufacturing/core/PrintIssue.h"
#include "kernel/manufacturing/core/PrintIssueType.h"
#include "kernel/manufacturing/mesh/AnalysisMesh.h"
#include "kernel/math/Bounds.h"
#include "kernel/math/Intersections.h"
#include "kernel/math/Ray.h"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Detects intersections between topologically unrelated regions of
     * an editable mesh.
     *
     * Analysis is performed over the canonical triangulation stored in
     * AnalysisMesh. Its BVH is used as a broad phase to reject geometrically
     * distant face pairs before exact triangle-triangle testing.
     *
     * Triangles belonging to the same source face are ignored. Faces sharing
     * an editable edge are also excluded because their expected topological
     * contact is not a self-intersection.
     *
     * A source-face pair produces at most one PrintIssue even when multiple
     * derived triangle pairs intersect.
     */
    class SelfIntersectionAnalyzer final : public IAnalyzer {
    public:
        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "SelfIntersectionAnalyzer";
        }

        /**
         * @brief Searches for intersecting non-adjacent surface regions.
         *
         * Both the authoritative LEM and its canonical AnalysisMesh are
         * required. Existing report contents are preserved.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving self-intersection issues.
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
                !analysisMesh.has_bvh()) {
                return;
            }

            std::vector<FacePair> reportedPairs;

            for (std::size_t triangleIndex = 0;
                triangleIndex < analysisMesh.triangle_count();
                ++triangleIndex) {

                analyze_triangle(
                    mesh,
                    analysisMesh,
                    triangleIndex,
                    reportedPairs,
                    report);
            }
        }

    private:
        /**
         * @brief Pair of source editable faces.
         */
        struct FacePair {
            geometry::FaceHandle first{};
            geometry::FaceHandle second{};
        };

        /**
         * @brief Intersection result returned by narrow-phase triangle tests.
         */
        struct TriangleIntersection {
            bool intersects = false;
            glm::vec3 sample{ 0.0f, 0.0f, 0.0f };
        };

        /**
         * @brief Numeric tolerance used by geometric intersection tests.
         */
        static constexpr float IntersectionEpsilon = 1.0e-6f;

        /**
         * @brief Performs broad- and narrow-phase analysis for one canonical
         * analysis triangle.
         *
         * @param mesh Source editable mesh.
         * @param analysisMesh Canonical manufacturing analysis mesh.
         * @param triangleIndex Triangle to inspect.
         * @param reportedPairs Face pairs already reported.
         * @param report Report receiving new issues.
         */
        static void analyze_triangle(
            const geometry::LEM& mesh,
            const AnalysisMesh& analysisMesh,
            std::size_t triangleIndex,
            std::vector<FacePair>& reportedPairs,
            AnalysisReport& report)
        {
            const geometry::FaceHandle sourceFace =
                analysisMesh.mapping()
                .face_for_triangle(triangleIndex);

            if (!mesh.is_valid(sourceFace)) {
                return;
            }

            const AnalysisTriangle& sourceTriangle =
                analysisMesh.triangle(
                    static_cast<AnalysisIndex>(
                        triangleIndex));

            if (!triangle_indices_valid(
                analysisMesh,
                sourceTriangle)) {
                return;
            }

            const glm::vec3& sourceA =
                analysisMesh.vertex(
                    sourceTriangle.a).position;

            const glm::vec3& sourceB =
                analysisMesh.vertex(
                    sourceTriangle.b).position;

            const glm::vec3& sourceC =
                analysisMesh.vertex(
                    sourceTriangle.c).position;

            const math::Bounds sourceBounds =
                triangle_bounds(
                    sourceA,
                    sourceB,
                    sourceC);

            const std::vector<geometry::FaceHandle> candidateFaces =
                geometry::BVHQuery::query_bounds(
                    analysisMesh.bvh(),
                    sourceBounds);

            for (const geometry::FaceHandle candidateFace :
            candidateFaces) {

                if (!mesh.is_valid(candidateFace) ||
                    candidateFace == sourceFace) {
                    continue;
                }

                if (faces_share_edge(
                    mesh,
                    sourceFace,
                    candidateFace)) {
                    continue;
                }

                if (pair_exists(
                    reportedPairs,
                    sourceFace,
                    candidateFace)) {
                    continue;
                }

                const TriangleIntersection intersection =
                    intersect_face_with_triangle(
                        analysisMesh,
                        triangleIndex,
                        candidateFace);

                if (!intersection.intersects) {
                    continue;
                }

                reportedPairs.push_back(
                    FacePair{
                        sourceFace,
                        candidateFace
                    });

                report_intersection(
                    sourceFace,
                    candidateFace,
                    intersection.sample,
                    report);
            }
        }

        /**
         * @brief Tests one source triangle against every canonical triangle
         * belonging to a candidate source face.
         *
         * @param analysisMesh Canonical analysis representation.
         * @param sourceTriangleIndex Source triangle index.
         * @param candidateFace Candidate editable face.
         * @return First detected triangle intersection.
         */
        [[nodiscard]] static TriangleIntersection
            intersect_face_with_triangle(
                const AnalysisMesh& analysisMesh,
                std::size_t sourceTriangleIndex,
                geometry::FaceHandle candidateFace)
        {
            const AnalysisTriangle& sourceTriangle =
                analysisMesh.triangle(
                    static_cast<AnalysisIndex>(
                        sourceTriangleIndex));

            const glm::vec3& a0 =
                analysisMesh.vertex(
                    sourceTriangle.a).position;

            const glm::vec3& a1 =
                analysisMesh.vertex(
                    sourceTriangle.b).position;

            const glm::vec3& a2 =
                analysisMesh.vertex(
                    sourceTriangle.c).position;

            for (std::size_t candidateTriangleIndex = 0;
                candidateTriangleIndex <
                analysisMesh.triangle_count();
                ++candidateTriangleIndex) {

                if (analysisMesh.mapping()
                    .face_for_triangle(
                        candidateTriangleIndex) !=
                    candidateFace) {
                    continue;
                }

                const AnalysisTriangle& candidateTriangle =
                    analysisMesh.triangle(
                        static_cast<AnalysisIndex>(
                            candidateTriangleIndex));

                if (!triangle_indices_valid(
                    analysisMesh,
                    candidateTriangle)) {
                    continue;
                }

                const glm::vec3& b0 =
                    analysisMesh.vertex(
                        candidateTriangle.a).position;

                const glm::vec3& b1 =
                    analysisMesh.vertex(
                        candidateTriangle.b).position;

                const glm::vec3& b2 =
                    analysisMesh.vertex(
                        candidateTriangle.c).position;

                const TriangleIntersection result =
                    intersect_triangles(
                        a0,
                        a1,
                        a2,
                        b0,
                        b1,
                        b2);

                if (result.intersects) {
                    return result;
                }
            }

            return {};
        }

        /**
         * @brief Tests two triangles for geometric intersection.
         *
         * Non-coplanar triangles are tested by intersecting each triangle edge
         * segment against the opposite triangle. Coplanar triangles are
         * projected onto their dominant plane and tested in two dimensions.
         *
         * @return Intersection state and a representative spatial sample.
         */
        [[nodiscard]] static TriangleIntersection intersect_triangles(
            const glm::vec3& a0,
            const glm::vec3& a1,
            const glm::vec3& a2,
            const glm::vec3& b0,
            const glm::vec3& b1,
            const glm::vec3& b2)
        {
            const glm::vec3 normalA =
                glm::cross(
                    a1 - a0,
                    a2 - a0);

            const glm::vec3 normalB =
                glm::cross(
                    b1 - b0,
                    b2 - b0);

            const float normalALength =
                glm::length(normalA);

            const float normalBLength =
                glm::length(normalB);

            if (normalALength <= IntersectionEpsilon ||
                normalBLength <= IntersectionEpsilon) {
                return {};
            }

            const glm::vec3 normalizedA =
                normalA / normalALength;

            const glm::vec3 normalizedB =
                normalB / normalBLength;

            const float normalAlignment =
                std::abs(
                    glm::dot(
                        normalizedA,
                        normalizedB));

            const float planeDistance =
                std::abs(
                    glm::dot(
                        b0 - a0,
                        normalizedA));

            const bool coplanar =
                normalAlignment >=
                1.0f - IntersectionEpsilon &&
                planeDistance <=
                IntersectionEpsilon;

            if (coplanar) {
                return intersect_coplanar_triangles(
                    a0,
                    a1,
                    a2,
                    b0,
                    b1,
                    b2,
                    normalizedA);
            }

            const std::array<
                std::pair<glm::vec3, glm::vec3>,
                3> edgesA{
                    std::pair{a0, a1},
                    std::pair{a1, a2},
                    std::pair{a2, a0}
            };

            const std::array<
                std::pair<glm::vec3, glm::vec3>,
                3> edgesB{
                    std::pair{b0, b1},
                    std::pair{b1, b2},
                    std::pair{b2, b0}
            };

            for (const auto& edge : edgesA) {
                const TriangleIntersection result =
                    intersect_segment_triangle(
                        edge.first,
                        edge.second,
                        b0,
                        b1,
                        b2);

                if (result.intersects) {
                    return result;
                }
            }

            for (const auto& edge : edgesB) {
                const TriangleIntersection result =
                    intersect_segment_triangle(
                        edge.first,
                        edge.second,
                        a0,
                        a1,
                        a2);

                if (result.intersects) {
                    return result;
                }
            }

            return {};
        }

        /**
         * @brief Intersects a finite line segment with a triangle.
         *
         * The kernel's ray-triangle routine is reused with the segment delta
         * as ray direction. In that parameterization, valid segment hits have
         * distances between zero and one.
         */
        [[nodiscard]] static TriangleIntersection
            intersect_segment_triangle(
                const glm::vec3& start,
                const glm::vec3& end,
                const glm::vec3& a,
                const glm::vec3& b,
                const glm::vec3& c)
        {
            const glm::vec3 direction =
                end - start;

            if (glm::length(direction) <=
                IntersectionEpsilon) {
                return {};
            }

            math::Ray ray;
            ray.origin = start;
            ray.direction = direction;

            const math::RayHit hit =
                math::intersect_ray_triangle(
                    ray,
                    a,
                    b,
                    c,
                    IntersectionEpsilon);

            if (!hit.hit) {
                return {};
            }

            if (hit.distance <
                -IntersectionEpsilon ||
                hit.distance >
                1.0f + IntersectionEpsilon) {
                return {};
            }

            return TriangleIntersection{
                true,
                hit.position
            };
        }

        /**
         * @brief Tests coplanar triangles after projection to two dimensions.
         *
         * @param normal Shared triangle plane normal.
         */
        [[nodiscard]] static TriangleIntersection
            intersect_coplanar_triangles(
                const glm::vec3& a0,
                const glm::vec3& a1,
                const glm::vec3& a2,
                const glm::vec3& b0,
                const glm::vec3& b1,
                const glm::vec3& b2,
                const glm::vec3& normal)
        {
            const int droppedAxis =
                dominant_axis(normal);

            const std::array<glm::vec2, 3> triangleA{
                project_to_2d(a0, droppedAxis),
                project_to_2d(a1, droppedAxis),
                project_to_2d(a2, droppedAxis)
            };

            const std::array<glm::vec2, 3> triangleB{
                project_to_2d(b0, droppedAxis),
                project_to_2d(b1, droppedAxis),
                project_to_2d(b2, droppedAxis)
            };

            for (std::size_t aEdge = 0;
                aEdge < 3;
                ++aEdge) {

                const glm::vec2& aStart =
                    triangleA[aEdge];

                const glm::vec2& aEnd =
                    triangleA[
                        (aEdge + 1) % 3];

                for (std::size_t bEdge = 0;
                    bEdge < 3;
                    ++bEdge) {

                    const glm::vec2& bStart =
                        triangleB[bEdge];

                    const glm::vec2& bEnd =
                        triangleB[
                            (bEdge + 1) % 3];

                    if (segments_intersect_2d(
                        aStart,
                        aEnd,
                        bStart,
                        bEnd)) {

                        const glm::vec3 sample =
                            (a0 + a1 + a2 +
                                b0 + b1 + b2) /
                            6.0f;

                        return TriangleIntersection{
                            true,
                            sample
                        };
                    }
                }
            }

            if (point_in_triangle_2d(
                triangleA[0],
                triangleB[0],
                triangleB[1],
                triangleB[2])) {

                return TriangleIntersection{
                    true,
                    a0
                };
            }

            if (point_in_triangle_2d(
                triangleB[0],
                triangleA[0],
                triangleA[1],
                triangleA[2])) {

                return TriangleIntersection{
                    true,
                    b0
                };
            }

            return {};
        }

        /**
         * @brief Returns the dominant normal axis to omit during 2D
         * projection.
         */
        [[nodiscard]] static int dominant_axis(
            const glm::vec3& normal) noexcept
        {
            const glm::vec3 absolute{
                std::abs(normal.x),
                std::abs(normal.y),
                std::abs(normal.z)
            };

            if (absolute.x >= absolute.y &&
                absolute.x >= absolute.z) {
                return 0;
            }

            if (absolute.y >= absolute.x &&
                absolute.y >= absolute.z) {
                return 1;
            }

            return 2;
        }

        /**
         * @brief Projects a 3D position by dropping one coordinate axis.
         */
        [[nodiscard]] static glm::vec2 project_to_2d(
            const glm::vec3& position,
            int droppedAxis) noexcept
        {
            switch (droppedAxis) {
            case 0:
                return glm::vec2{
                    position.y,
                    position.z
                };

            case 1:
                return glm::vec2{
                    position.x,
                    position.z
                };

            default:
                return glm::vec2{
                    position.x,
                    position.y
                };
            }
        }

        /**
         * @brief Returns the signed 2D orientation of three points.
         */
        [[nodiscard]] static float orientation_2d(
            const glm::vec2& a,
            const glm::vec2& b,
            const glm::vec2& c) noexcept
        {
            return
                (b.x - a.x) *
                (c.y - a.y) -
                (b.y - a.y) *
                (c.x - a.x);
        }

        /**
         * @brief Checks whether a point lies on a 2D segment.
         */
        [[nodiscard]] static bool point_on_segment_2d(
            const glm::vec2& point,
            const glm::vec2& start,
            const glm::vec2& end) noexcept
        {
            if (std::abs(
                orientation_2d(
                    start,
                    end,
                    point)) >
                IntersectionEpsilon) {
                return false;
            }

            return
                point.x >=
                std::min(start.x, end.x) -
                IntersectionEpsilon &&
                point.x <=
                std::max(start.x, end.x) +
                IntersectionEpsilon &&
                point.y >=
                std::min(start.y, end.y) -
                IntersectionEpsilon &&
                point.y <=
                std::max(start.y, end.y) +
                IntersectionEpsilon;
        }

        /**
         * @brief Tests two finite 2D segments for intersection.
         */
        [[nodiscard]] static bool segments_intersect_2d(
            const glm::vec2& a0,
            const glm::vec2& a1,
            const glm::vec2& b0,
            const glm::vec2& b1) noexcept
        {
            const float o1 =
                orientation_2d(a0, a1, b0);

            const float o2 =
                orientation_2d(a0, a1, b1);

            const float o3 =
                orientation_2d(b0, b1, a0);

            const float o4 =
                orientation_2d(b0, b1, a1);

            if (((o1 > IntersectionEpsilon &&
                o2 < -IntersectionEpsilon) ||
                (o1 < -IntersectionEpsilon &&
                    o2 > IntersectionEpsilon)) &&
                ((o3 > IntersectionEpsilon &&
                    o4 < -IntersectionEpsilon) ||
                    (o3 < -IntersectionEpsilon &&
                        o4 > IntersectionEpsilon))) {

                return true;
            }

            if (std::abs(o1) <= IntersectionEpsilon &&
                point_on_segment_2d(
                    b0,
                    a0,
                    a1)) {
                return true;
            }

            if (std::abs(o2) <= IntersectionEpsilon &&
                point_on_segment_2d(
                    b1,
                    a0,
                    a1)) {
                return true;
            }

            if (std::abs(o3) <= IntersectionEpsilon &&
                point_on_segment_2d(
                    a0,
                    b0,
                    b1)) {
                return true;
            }

            if (std::abs(o4) <= IntersectionEpsilon &&
                point_on_segment_2d(
                    a1,
                    b0,
                    b1)) {
                return true;
            }

            return false;
        }

        /**
         * @brief Checks whether a 2D point lies inside or on a triangle.
         */
        [[nodiscard]] static bool point_in_triangle_2d(
            const glm::vec2& point,
            const glm::vec2& a,
            const glm::vec2& b,
            const glm::vec2& c) noexcept
        {
            const float o1 =
                orientation_2d(a, b, point);

            const float o2 =
                orientation_2d(b, c, point);

            const float o3 =
                orientation_2d(c, a, point);

            const bool hasNegative =
                o1 < -IntersectionEpsilon ||
                o2 < -IntersectionEpsilon ||
                o3 < -IntersectionEpsilon;

            const bool hasPositive =
                o1 > IntersectionEpsilon ||
                o2 > IntersectionEpsilon ||
                o3 > IntersectionEpsilon;

            return !(hasNegative &&
                hasPositive);
        }

        /**
         * @brief Checks whether a canonical triangle references valid
         * AnalysisMesh vertices.
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
         * @brief Computes bounds enclosing one triangle.
         */
        [[nodiscard]] static math::Bounds triangle_bounds(
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c)
        {
            math::Bounds bounds =
                math::Bounds::empty();

            bounds.expand(a);
            bounds.expand(b);
            bounds.expand(c);

            return bounds;
        }

        /**
         * @brief Checks whether two editable faces share at least one edge.
         *
         * Expected topological contact across that edge is excluded from
         * self-intersection analysis.
         */
        [[nodiscard]] static bool faces_share_edge(
            const geometry::LEM& mesh,
            geometry::FaceHandle first,
            geometry::FaceHandle second)
        {
            const std::vector<geometry::EdgeHandle> firstEdges =
                geometry::TopologyTraversal::face_edges(
                    mesh,
                    first);

            const std::vector<geometry::EdgeHandle> secondEdges =
                geometry::TopologyTraversal::face_edges(
                    mesh,
                    second);

            for (const geometry::EdgeHandle firstEdge :
            firstEdges) {

                if (std::find(
                    secondEdges.begin(),
                    secondEdges.end(),
                    firstEdge) !=
                    secondEdges.end()) {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Checks whether an unordered face pair was already reported.
         */
        [[nodiscard]] static bool pair_exists(
            const std::vector<FacePair>& pairs,
            geometry::FaceHandle first,
            geometry::FaceHandle second)
        {
            for (const FacePair& pair : pairs) {
                if ((pair.first == first &&
                    pair.second == second) ||
                    (pair.first == second &&
                        pair.second == first)) {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Reports one intersecting source-face pair.
         *
         * Both faces and an approximate intersection sample are retained for
         * future editor visualization.
         */
        static void report_intersection(
            geometry::FaceHandle first,
            geometry::FaceHandle second,
            const glm::vec3& sample,
            AnalysisReport& report)
        {
            IssueLocation location;

            location.faces.push_back(first);
            location.faces.push_back(second);

            location.samples.push_back(sample);

            PrintIssue issue{
                PrintIssueType::SelfIntersection,
                IssueSeverity::Error,
                "Non-adjacent surface regions intersect.",
                std::move(location)
            };

            report.add_issue(
                std::move(issue));
        }
    };

}