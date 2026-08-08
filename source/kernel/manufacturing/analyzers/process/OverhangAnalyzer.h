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
#include "kernel/manufacturing/profiles/PrintProfile.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Detects downward-facing surface regions whose overhang angle
     * exceeds the manufacturing profile limit.
     *
     * Overhang angle is measured from a vertical wall:
     *
     * - 0 degrees represents a vertical surface;
     * - 90 degrees represents a horizontal downward-facing surface.
     *
     * Upward-facing surfaces are not classified as overhangs.
     *
     * The analyzer identifies geometric overhangs only. Whether those regions
     * actually require support remains the responsibility of
     * SupportRequirementAnalyzer.
     */
    class OverhangAnalyzer final : public IAnalyzer {
    public:
        /**
         * @brief Creates an overhang analyzer.
         *
         * @param buildDirection Direction in which layers are built.
         * Defaults to positive Z.
         */
        explicit OverhangAnalyzer(
            const glm::vec3& buildDirection =
            glm::vec3{ 0.0f, 0.0f, 1.0f }) noexcept
            : buildDirection_(
                normalized_build_direction(
                    buildDirection))
        {
        }

        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "OverhangAnalyzer";
        }

        /**
         * @brief Returns the normalized build direction.
         *
         * @return Build direction used by this analyzer.
         */
        [[nodiscard]] const glm::vec3& build_direction() const noexcept
        {
            return buildDirection_;
        }

        /**
         * @brief Detects process-dependent overhang regions.
         *
         * A canonical AnalysisMesh and a PrintProfile containing
         * maximumUnsupportedOverhangAngleDegrees are required.
         *
         * Existing report contents are preserved.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving overhang issues.
         */
        void analyze(
            const AnalysisContext& context,
            AnalysisReport& report) const override
        {
            if (!context.has_mesh() ||
                !context.has_analysis_mesh() ||
                !context.has_profile()) {
                return;
            }

            const double limit =
                configured_limit(
                    *context.profile);

            if (!std::isfinite(limit) ||
                limit < 0.0 ||
                limit > 90.0) {
                return;
            }

            const geometry::LEM& mesh =
                *context.mesh;

            const AnalysisMesh& analysisMesh =
                *context.analysisMesh;

            if (analysisMesh.empty()) {
                return;
            }

            const std::vector<FaceOverhang> overhangs =
                collect_overhang_faces(
                    mesh,
                    analysisMesh,
                    limit);

            if (overhangs.empty()) {
                return;
            }

            report_regions(
                mesh,
                overhangs,
                limit,
                report);
        }

    private:
        /**
         * @brief Process result accumulated for one editable face.
         */
        struct FaceOverhang {
            geometry::FaceHandle face{};
            double maximumAngleDegrees = 0.0;
            std::vector<glm::vec3> samples{};
        };

        /**
         * @brief Small tolerance used for degenerate triangle rejection.
         */
        static constexpr float NormalEpsilon = 1.0e-8f;

        /**
         * @brief Conversion constant from radians to degrees.
         */
        static constexpr double RadiansToDegrees =
            57.2957795130823208768;

        /**
         * @brief Finds faces containing at least one overhanging canonical
         * triangle.
         */
        [[nodiscard]] std::vector<FaceOverhang>
            collect_overhang_faces(
                const geometry::LEM& mesh,
                const AnalysisMesh& analysisMesh,
                double limit) const
        {
            std::vector<FaceOverhang> result;

            const float buildPlaneProjection =
                minimum_build_plane_projection(
                    analysisMesh);

            const float buildPlaneTolerance =
                build_plane_tolerance(
                    analysisMesh);

            for (std::size_t triangleIndex = 0;
                triangleIndex < analysisMesh.triangle_count();
                ++triangleIndex) {

                const geometry::FaceHandle faceHandle =
                    analysisMesh.mapping()
                    .face_for_triangle(
                        triangleIndex);

                if (!mesh.is_valid(faceHandle)) {
                    continue;
                }

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

                if (triangle_on_build_plane(
                    a,
                    b,
                    c,
                    buildPlaneProjection,
                    buildPlaneTolerance)) {
                    continue;
                }

                const glm::vec3 cross =
                    glm::cross(
                        b - a,
                        c - a);

                const float length =
                    glm::length(cross);

                if (!std::isfinite(length) ||
                    length <= NormalEpsilon) {
                    continue;
                }

                const glm::vec3 normal =
                    cross / length;

                const double angle =
                    overhang_angle_degrees(
                        normal);

                if (!std::isfinite(angle) ||
                    angle <= limit) {
                    continue;
                }

                const glm::vec3 sample =
                    (a + b + c) / 3.0f;

                FaceOverhang* existing =
                    find_face(
                        result,
                        faceHandle);

                if (existing == nullptr) {
                    FaceOverhang faceOverhang;

                    faceOverhang.face =
                        faceHandle;

                    faceOverhang.maximumAngleDegrees =
                        angle;

                    faceOverhang.samples.push_back(
                        sample);

                    result.push_back(
                        std::move(faceOverhang));

                    continue;
                }

                existing->maximumAngleDegrees =
                    std::max(
                        existing->maximumAngleDegrees,
                        angle);

                existing->samples.push_back(
                    sample);
            }

            return result;
        }

        /**
         * @brief Computes the lowest model projection along the build axis.
         */
        [[nodiscard]] float minimum_build_plane_projection(
            const AnalysisMesh& analysisMesh) const noexcept
        {
            if (!analysisMesh.has_bounds()) {
                return
                    std::numeric_limits<float>::quiet_NaN();
            }

            const math::Bounds& bounds =
                analysisMesh.bounds();

            const std::array<glm::vec3, 8> corners{
                glm::vec3{ bounds.min.x, bounds.min.y, bounds.min.z },
                glm::vec3{ bounds.min.x, bounds.min.y, bounds.max.z },
                glm::vec3{ bounds.min.x, bounds.max.y, bounds.min.z },
                glm::vec3{ bounds.min.x, bounds.max.y, bounds.max.z },
                glm::vec3{ bounds.max.x, bounds.min.y, bounds.min.z },
                glm::vec3{ bounds.max.x, bounds.min.y, bounds.max.z },
                glm::vec3{ bounds.max.x, bounds.max.y, bounds.min.z },
                glm::vec3{ bounds.max.x, bounds.max.y, bounds.max.z }
            };

            float minimum =
                std::numeric_limits<float>::max();

            for (const glm::vec3& corner :
            corners) {
                minimum =
                    std::min(
                        minimum,
                        glm::dot(
                            corner,
                            buildDirection_));
            }

            return minimum;
        }

        /**
         * @brief Returns a scale-aware tolerance for build-plane contact.
         */
        [[nodiscard]] static float build_plane_tolerance(
            const AnalysisMesh& analysisMesh) noexcept
        {
            if (!analysisMesh.has_bounds()) {
                return 0.0f;
            }

            const float diagonal =
                glm::length(
                    analysisMesh.bounds().size());

            return
                std::max(
                    1.0e-5f,
                    diagonal * 1.0e-5f);
        }

        /**
         * @brief Checks whether an overhang candidate is on the build plate.
         */
        [[nodiscard]] bool triangle_on_build_plane(
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c,
            float buildPlaneProjection,
            float tolerance) const noexcept
        {
            if (!std::isfinite(buildPlaneProjection)) {
                return false;
            }

            return
                glm::dot(
                    a,
                    buildDirection_) <= buildPlaneProjection + tolerance &&
                glm::dot(
                    b,
                    buildDirection_) <= buildPlaneProjection + tolerance &&
                glm::dot(
                    c,
                    buildDirection_) <= buildPlaneProjection + tolerance;
        }

        /**
         * @brief Computes the overhang angle represented by one surface normal.
         *
         * Upward-facing and vertical surfaces return zero.
         */
        [[nodiscard]] double overhang_angle_degrees(
            const glm::vec3& normal) const noexcept
        {
            const double downwardAmount =
                -static_cast<double>(
                    glm::dot(
                        normal,
                        buildDirection_));

            if (downwardAmount <= 0.0) {
                return 0.0;
            }

            const double clamped =
                glm::clamp(
                    downwardAmount,
                    0.0,
                    1.0);

            return
                std::asin(clamped) *
                RadiansToDegrees;
        }

        /**
         * @brief Groups adjacent overhang faces into semantic regions.
         */
        static void report_regions(
            const geometry::LEM& mesh,
            const std::vector<FaceOverhang>& overhangs,
            double limit,
            AnalysisReport& report)
        {
            std::vector<geometry::FaceHandle> visited;
            visited.reserve(
                overhangs.size());

            for (const FaceOverhang& start :
                overhangs) {

                if (contains(
                    visited,
                    start.face)) {
                    continue;
                }

                std::vector<const FaceOverhang*> region;
                std::vector<geometry::FaceHandle> pending;

                pending.push_back(
                    start.face);

                while (!pending.empty()) {
                    const geometry::FaceHandle current =
                        pending.back();

                    pending.pop_back();

                    if (contains(
                        visited,
                        current)) {
                        continue;
                    }

                    const FaceOverhang* currentOverhang =
                        find_face(
                            overhangs,
                            current);

                    if (currentOverhang == nullptr) {
                        continue;
                    }

                    visited.push_back(
                        current);

                    region.push_back(
                        currentOverhang);

                    const std::vector<geometry::EdgeHandle> edges =
                        geometry::TopologyTraversal::face_edges(
                            mesh,
                            current);

                    for (const geometry::EdgeHandle edgeHandle :
                    edges) {

                        if (!mesh.is_valid(edgeHandle)) {
                            continue;
                        }

                        const std::vector<geometry::FaceHandle> adjacent =
                            geometry::TopologyTraversal::edge_faces(
                                mesh,
                                edgeHandle);

                        for (const geometry::FaceHandle adjacentFace :
                        adjacent) {

                            if (!contains(
                                visited,
                                adjacentFace) &&
                                find_face(
                                    overhangs,
                                    adjacentFace) != nullptr) {

                                pending.push_back(
                                    adjacentFace);
                            }
                        }
                    }
                }

                emit_region(
                    mesh,
                    region,
                    limit,
                    report);
            }
        }

        /**
         * @brief Emits one grouped overhang issue.
         */
        static void emit_region(
            const geometry::LEM& mesh,
            const std::vector<const FaceOverhang*>& region,
            double limit,
            AnalysisReport& report)
        {
            if (region.empty()) {
                return;
            }

            IssueLocation location;

            double maximumAngle =
                0.0;

            for (const FaceOverhang* faceOverhang :
                region) {

                if (faceOverhang == nullptr ||
                    !mesh.is_valid(
                        faceOverhang->face)) {
                    continue;
                }

                add_unique(
                    location.faces,
                    faceOverhang->face);

                maximumAngle =
                    std::max(
                        maximumAngle,
                        faceOverhang
                        ->maximumAngleDegrees);

                for (const glm::vec3& sample :
                    faceOverhang->samples) {

                    location.samples.push_back(
                        sample);

                    location.region.expand(
                        sample);
                }

                const std::vector<geometry::EdgeHandle> edges =
                    geometry::TopologyTraversal::face_edges(
                        mesh,
                        faceOverhang->face);

                for (const geometry::EdgeHandle edgeHandle :
                edges) {

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
                }
            }

            if (location.faces.empty()) {
                return;
            }

            PrintIssue issue{
                PrintIssueType::Overhang,
                IssueSeverity::Warning,
                "Surface overhang angle exceeds the configured manufacturing limit.",
                std::move(location)
            };

            issue.measurement =
                IssueMeasurement{
                    IssueMeasurementKind::AngleDegrees,
                    maximumAngle,
                    limit
            };

            report.add_issue(
                std::move(issue));
        }

        /**
         * @brief Returns the configured profile limit.
         */
        [[nodiscard]] static double configured_limit(
            const PrintProfile& profile)
        {
            const auto& limit =
                profile.limits()
                .maximumUnsupportedOverhangAngleDegrees;

            if (!limit.has_value()) {
                return
                    std::numeric_limits<double>::quiet_NaN();
            }

            return limit.value();
        }

        /**
         * @brief Normalizes build direction, falling back to +Z for invalid
         * input.
         */
        [[nodiscard]] static glm::vec3 normalized_build_direction(
            const glm::vec3& direction) noexcept
        {
            const float length =
                glm::length(direction);

            if (!std::isfinite(length) ||
                length <= NormalEpsilon) {

                return
                    glm::vec3{
                        0.0f,
                        0.0f,
                        1.0f
                };
            }

            return direction / length;
        }

        /**
         * @brief Checks canonical triangle indices.
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
         * @brief Finds mutable accumulated data for a face.
         */
        [[nodiscard]] static FaceOverhang* find_face(
            std::vector<FaceOverhang>& faces,
            geometry::FaceHandle faceHandle)
        {
            for (FaceOverhang& face : faces) {
                if (face.face == faceHandle) {
                    return &face;
                }
            }

            return nullptr;
        }

        /**
         * @brief Finds immutable accumulated data for a face.
         */
        [[nodiscard]] static const FaceOverhang* find_face(
            const std::vector<FaceOverhang>& faces,
            geometry::FaceHandle faceHandle)
        {
            for (const FaceOverhang& face : faces) {
                if (face.face == faceHandle) {
                    return &face;
                }
            }

            return nullptr;
        }

        /**
         * @brief Checks whether a value occurs in a collection.
         */
        template <typename T>
        [[nodiscard]] static bool contains(
            const std::vector<T>& values,
            const T& value)
        {
            return
                std::find(
                    values.begin(),
                    values.end(),
                    value) !=
                values.end();
        }

        /**
         * @brief Adds a value once.
         */
        template <typename T>
        static void add_unique(
            std::vector<T>& values,
            const T& value)
        {
            if (!contains(
                values,
                value)) {

                values.push_back(
                    value);
            }
        }

        glm::vec3 buildDirection_{
            0.0f,
            0.0f,
            1.0f
        };
    };

}
