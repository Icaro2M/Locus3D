/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/queries/SelectionHit.h"
#include "kernel/geometry/spatial/BVHQuery.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/manufacturing/analyzers/thinwall/IThinWallAnalyzer.h"
#include "kernel/manufacturing/analyzers/thinwall/ThinWallResult.h"
#include "kernel/manufacturing/core/AnalysisContext.h"
#include "kernel/manufacturing/core/AnalysisReport.h"
#include "kernel/manufacturing/core/IssueLocation.h"
#include "kernel/manufacturing/core/IssueMeasurement.h"
#include "kernel/manufacturing/core/IssueSeverity.h"
#include "kernel/manufacturing/core/PrintIssue.h"
#include "kernel/manufacturing/core/PrintIssueType.h"
#include "kernel/manufacturing/mesh/AnalysisMesh.h"
#include "kernel/manufacturing/profiles/PrintProfile.h"
#include "kernel/math/Ray.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Detects walls thinner than the configured manufacturing limit
     * using BVH-accelerated surface raycasts.
     *
     * Each canonical AnalysisMesh triangle is sampled at one or more interior
     * points. Rays are emitted in both directions along the triangle normal and
     * the nearest compatible opposite surface is interpreted as a local wall
     * thickness measurement.
     *
     * Fast, Balanced, and High quality modes use the same geometric model and
     * differ only in sampling density.
     */
    class RaycastThinWallAnalyzer final : public IThinWallAnalyzer {
    public:
        /**
         * @brief Creates a raycast thin-wall analyzer.
         *
         * @param quality Requested analysis quality.
         */
        explicit RaycastThinWallAnalyzer(
            ThinWallQuality quality = ThinWallQuality::Balanced) noexcept
            : quality_(quality)
        {
        }

        /**
         * @brief Returns the stable analyzer name.
         *
         * @return Analyzer identifier.
         */
        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "RaycastThinWallAnalyzer";
        }

        /**
         * @brief Returns the configured thin-wall quality.
         *
         * @return Analysis quality.
         */
        [[nodiscard]] ThinWallQuality quality() const noexcept override
        {
            return quality_;
        }

        /**
         * @brief Detects regions thinner than the active profile limit.
         *
         * A source LEM, canonical AnalysisMesh, valid BVH, and positive
         * minimumWallThickness profile limit are required.
         *
         * Existing report contents are preserved.
         *
         * @param context Manufacturing analysis inputs.
         * @param report Report receiving thin-wall issues.
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
                minimum_wall_thickness(
                    *context.profile);

            if (!std::isfinite(limit) ||
                limit <= 0.0) {
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

            const std::vector<FaceComponent> components =
                build_face_components(mesh);

            std::vector<ThinWallResult> results;

            collect_results(
                mesh,
                analysisMesh,
                components,
                limit,
                results);

            if (results.empty()) {
                return;
            }

            report_results(
                mesh,
                results,
                limit,
                report);
        }

    private:
        /**
         * @brief Connected group of editable faces.
         */
        struct FaceComponent {
            std::vector<geometry::FaceHandle> faces{};
        };

        /**
         * @brief Interior triangle sample represented by barycentric weights.
         */
        struct BarycentricSample {
            float a = 0.0f;
            float b = 0.0f;
            float c = 0.0f;
        };

        /**
         * @brief Minimum useful measured thickness.
         *
         * Values below this tolerance belong to degenerate geometry rather
         * than process-specific thin-wall analysis.
         */
        static constexpr double DegenerateThicknessEpsilon = 1.0e-9;

        /**
         * @brief Maximum number of successive BVH hits skipped while searching
         * for a compatible opposite surface.
         */
        static constexpr std::size_t MaxSkippedHits = 8;

        /**
         * @brief Collects local thin-wall measurements.
         */
        void collect_results(
            const geometry::LEM& mesh,
            const AnalysisMesh& analysisMesh,
            const std::vector<FaceComponent>& components,
            double limit,
            std::vector<ThinWallResult>& results) const
        {
            for (std::size_t triangleIndex = 0;
                triangleIndex < analysisMesh.triangle_count();
                ++triangleIndex) {

                const geometry::FaceHandle sourceFace =
                    analysisMesh.mapping()
                    .face_for_triangle(triangleIndex);

                if (!mesh.is_valid(sourceFace)) {
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

                const glm::vec3 cross =
                    glm::cross(
                        b - a,
                        c - a);

                const float normalLength =
                    glm::length(cross);

                if (!std::isfinite(normalLength) ||
                    normalLength <=
                    static_cast<float>(
                        DegenerateThicknessEpsilon)) {
                    continue;
                }

                const glm::vec3 normal =
                    cross / normalLength;

                const std::vector<BarycentricSample> samples =
                    samples_for_quality();

                for (const BarycentricSample& sample :
                    samples) {

                    const glm::vec3 position =
                        a * sample.a +
                        b * sample.b +
                        c * sample.c;

                    const std::optional<ThinWallResult> result =
                        measure_sample(
                            mesh,
                            analysisMesh,
                            components,
                            sourceFace,
                            position,
                            normal,
                            limit);

                    if (result.has_value()) {
                        results.push_back(
                            result.value());
                    }
                }
            }
        }

        /**
         * @brief Measures one surface position in both normal directions.
         *
         * @return Best valid measurement below the configured limit.
         */
        [[nodiscard]] std::optional<ThinWallResult>
            measure_sample(
                const geometry::LEM& mesh,
                const AnalysisMesh& analysisMesh,
                const std::vector<FaceComponent>& components,
                geometry::FaceHandle sourceFace,
                const glm::vec3& position,
                const glm::vec3& normal,
                double limit) const
        {
            const std::optional<ThinWallResult> forward =
                cast_for_opposite_surface(
                    mesh,
                    analysisMesh,
                    components,
                    sourceFace,
                    position,
                    normal,
                    limit);

            const std::optional<ThinWallResult> backward =
                cast_for_opposite_surface(
                    mesh,
                    analysisMesh,
                    components,
                    sourceFace,
                    position,
                    -normal,
                    limit);

            if (forward.has_value() &&
                backward.has_value()) {

                return
                    forward->thickness <=
                    backward->thickness
                    ? forward
                    : backward;
            }

            if (forward.has_value()) {
                return forward;
            }

            return backward;
        }

        /**
         * @brief Casts toward one side of a surface looking for a compatible
         * opposite wall.
         */
        [[nodiscard]] std::optional<ThinWallResult>
            cast_for_opposite_surface(
                const geometry::LEM& mesh,
                const AnalysisMesh& analysisMesh,
                const std::vector<FaceComponent>& components,
                geometry::FaceHandle sourceFace,
                const glm::vec3& sourcePosition,
                const glm::vec3& direction,
                double limit) const
        {
            const float directionLength =
                glm::length(direction);

            if (!std::isfinite(directionLength) ||
                directionLength <= 0.0f) {
                return std::nullopt;
            }

            const glm::vec3 rayDirection =
                direction / directionLength;

            const float originOffset =
                ray_origin_offset(limit);

            glm::vec3 currentOrigin =
                sourcePosition +
                rayDirection * originOffset;

            double travelled =
                static_cast<double>(
                    originOffset);

            for (std::size_t attempt = 0;
                attempt < MaxSkippedHits;
                ++attempt) {

                const double remaining =
                    limit - travelled;

                if (remaining <= 0.0) {
                    return std::nullopt;
                }

                math::Ray ray;
                ray.origin = currentOrigin;
                ray.direction = rayDirection;

                const geometry::SelectionHit hit =
                    geometry::BVHQuery::raycast_faces(
                        analysisMesh.bvh(),
                        ray,
                        static_cast<float>(
                            remaining));

                if (!hit.is_face() ||
                    !std::isfinite(hit.distance)) {
                    return std::nullopt;
                }

                const double thickness =
                    travelled +
                    static_cast<double>(
                        hit.distance);

                /*
                 * Numerical re-hit of the source face. Move beyond the hit
                 * and continue searching instead of allowing it to hide the
                 * actual opposing wall.
                 */
                if (hit.face == sourceFace) {
                    advance_past_hit(
                        hit,
                        rayDirection,
                        originOffset,
                        currentOrigin,
                        travelled);

                    continue;
                }

                if (!same_component(
                    components,
                    sourceFace,
                    hit.face)) {

                    advance_past_hit(
                        hit,
                        rayDirection,
                        originOffset,
                        currentOrigin,
                        travelled);

                    continue;
                }

                /*
                 * Opposite wall normals should belong to opposite
                 * hemispheres. This rejects ordinary same-facing nearby
                 * surfaces.
                 */
                const glm::vec3 hitNormal =
                    normalized_or_zero(
                        hit.normal);

                if (glm::length(hitNormal) <= 0.0f ||
                    glm::dot(
                        rayDirection,
                        hitNormal) <= 0.0f) {

                    advance_past_hit(
                        hit,
                        rayDirection,
                        originOffset,
                        currentOrigin,
                        travelled);

                    continue;
                }

                if (!std::isfinite(thickness) ||
                    thickness <=
                    DegenerateThicknessEpsilon ||
                    thickness >= limit) {
                    return std::nullopt;
                }

                ThinWallResult result;

                result.sourceFace =
                    sourceFace;

                result.oppositeFace =
                    hit.face;

                result.sourcePosition =
                    sourcePosition;

                result.oppositePosition =
                    hit.position;

                result.direction =
                    rayDirection;

                result.thickness =
                    thickness;

                result.confidence =
                    measurement_confidence(
                        direction,
                        hitNormal);

                return result;
            }

            return std::nullopt;
        }

        /**
         * @brief Advances a ray origin beyond a rejected BVH hit.
         */
        static void advance_past_hit(
            const geometry::SelectionHit& hit,
            const glm::vec3& direction,
            float offset,
            glm::vec3& origin,
            double& travelled)
        {
            const float advance =
                std::max(
                    hit.distance,
                    0.0f) +
                offset;

            origin +=
                direction * advance;

            travelled +=
                static_cast<double>(
                    advance);
        }

        /**
         * @brief Reports connected groups of related thin-wall measurements.
         */
        static void report_results(
            const geometry::LEM& mesh,
            const std::vector<ThinWallResult>& results,
            double limit,
            AnalysisReport& report)
        {
            std::vector<std::size_t> visited;
            visited.reserve(results.size());

            for (std::size_t index = 0;
                index < results.size();
                ++index) {

                if (contains(
                    visited,
                    index)) {
                    continue;
                }

                std::vector<std::size_t> group;
                std::vector<std::size_t> pending;

                pending.push_back(index);

                while (!pending.empty()) {
                    const std::size_t current =
                        pending.back();

                    pending.pop_back();

                    if (contains(
                        visited,
                        current)) {
                        continue;
                    }

                    visited.push_back(current);
                    group.push_back(current);

                    for (std::size_t candidate = 0;
                        candidate < results.size();
                        ++candidate) {

                        if (contains(
                            visited,
                            candidate)) {
                            continue;
                        }

                        if (results_related(
                            mesh,
                            results[current],
                            results[candidate])) {

                            pending.push_back(
                                candidate);
                        }
                    }
                }

                emit_issue(
                    results,
                    group,
                    limit,
                    report);
            }
        }

        /**
         * @brief Emits one semantic thin-wall issue.
         */
        static void emit_issue(
            const std::vector<ThinWallResult>& results,
            const std::vector<std::size_t>& group,
            double limit,
            AnalysisReport& report)
        {
            if (group.empty()) {
                return;
            }

            IssueLocation location;

            double minimumThickness =
                std::numeric_limits<double>::infinity();

            for (const std::size_t index : group) {
                const ThinWallResult& result =
                    results[index];

                add_unique(
                    location.faces,
                    result.sourceFace);

                if (result.oppositeFace.is_valid()) {
                    add_unique(
                        location.faces,
                        result.oppositeFace);
                }

                location.samples.push_back(
                    result.sourcePosition);

                location.samples.push_back(
                    result.oppositePosition);

                location.region.expand(
                    result.sourcePosition);

                location.region.expand(
                    result.oppositePosition);

                minimumThickness =
                    std::min(
                        minimumThickness,
                        result.thickness);
            }

            if (!std::isfinite(minimumThickness)) {
                return;
            }

            PrintIssue issue{
                PrintIssueType::ThinWall,
                IssueSeverity::Warning,
                "Wall thickness is below the configured manufacturing limit.",
                std::move(location)
            };

            issue.measurement =
                IssueMeasurement{
                    IssueMeasurementKind::Length,
                    minimumThickness,
                    limit
            };

            report.add_issue(
                std::move(issue));
        }

        /**
         * @brief Checks whether two local measurements belong to one connected
         * thin-wall region.
         */
        [[nodiscard]] static bool results_related(
            const geometry::LEM& mesh,
            const ThinWallResult& first,
            const ThinWallResult& second)
        {
            const bool direct =
                faces_equal_or_adjacent(
                    mesh,
                    first.sourceFace,
                    second.sourceFace) &&
                faces_equal_or_adjacent(
                    mesh,
                    first.oppositeFace,
                    second.oppositeFace);

            if (direct) {
                return true;
            }

            /*
             * Same physical wall can be measured from both sides.
             */
            return
                faces_equal_or_adjacent(
                    mesh,
                    first.sourceFace,
                    second.oppositeFace) &&
                faces_equal_or_adjacent(
                    mesh,
                    first.oppositeFace,
                    second.sourceFace);
        }

        /**
         * @brief Checks equality or edge adjacency between two faces.
         */
        [[nodiscard]] static bool faces_equal_or_adjacent(
            const geometry::LEM& mesh,
            geometry::FaceHandle first,
            geometry::FaceHandle second)
        {
            if (!mesh.is_valid(first) ||
                !mesh.is_valid(second)) {
                return false;
            }

            if (first == second) {
                return true;
            }

            const std::vector<geometry::EdgeHandle> edges =
                geometry::TopologyTraversal::face_edges(
                    mesh,
                    first);

            for (const geometry::EdgeHandle edgeHandle :
            edges) {

                const std::vector<geometry::FaceHandle> adjacent =
                    geometry::TopologyTraversal::edge_faces(
                        mesh,
                        edgeHandle);

                if (std::find(
                    adjacent.begin(),
                    adjacent.end(),
                    second) !=
                    adjacent.end()) {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Builds connected source-face components.
         */
        [[nodiscard]] static std::vector<FaceComponent>
            build_face_components(
                const geometry::LEM& mesh)
        {
            std::vector<FaceComponent> components;
            std::vector<geometry::FaceHandle> visited;

            const std::vector<geometry::FaceHandle> faces =
                geometry::TopologyTraversal::faces(mesh);

            visited.reserve(
                faces.size());

            for (const geometry::FaceHandle start :
            faces) {

                if (!mesh.is_valid(start) ||
                    contains(
                        visited,
                        start)) {
                    continue;
                }

                FaceComponent component;
                std::vector<geometry::FaceHandle> pending;

                pending.push_back(start);

                while (!pending.empty()) {
                    const geometry::FaceHandle current =
                        pending.back();

                    pending.pop_back();

                    if (!mesh.is_valid(current) ||
                        contains(
                            visited,
                            current)) {
                        continue;
                    }

                    visited.push_back(current);
                    component.faces.push_back(current);

                    const std::vector<geometry::EdgeHandle> edges =
                        geometry::TopologyTraversal::face_edges(
                            mesh,
                            current);

                    for (const geometry::EdgeHandle edge :
                    edges) {

                        const std::vector<geometry::FaceHandle> adjacent =
                            geometry::TopologyTraversal::edge_faces(
                                mesh,
                                edge);

                        for (const geometry::FaceHandle face :
                        adjacent) {

                            if (mesh.is_valid(face) &&
                                !contains(
                                    visited,
                                    face)) {

                                pending.push_back(face);
                            }
                        }
                    }
                }

                if (!component.faces.empty()) {
                    components.push_back(
                        std::move(component));
                }
            }

            return components;
        }

        /**
         * @brief Checks whether two faces belong to the same connected shell.
         */
        [[nodiscard]] static bool same_component(
            const std::vector<FaceComponent>& components,
            geometry::FaceHandle first,
            geometry::FaceHandle second)
        {
            for (const FaceComponent& component :
                components) {

                if (contains(
                    component.faces,
                    first) &&
                    contains(
                        component.faces,
                        second)) {

                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Returns interior sample positions for the active quality.
         */
        [[nodiscard]] std::vector<BarycentricSample>
            samples_for_quality() const
        {
            std::vector<BarycentricSample> samples;

            samples.push_back(
                BarycentricSample{
                    1.0f / 3.0f,
                    1.0f / 3.0f,
                    1.0f / 3.0f
                });

            if (quality_ == ThinWallQuality::Fast) {
                return samples;
            }

            samples.push_back(
                BarycentricSample{
                    0.6f,
                    0.2f,
                    0.2f
                });

            samples.push_back(
                BarycentricSample{
                    0.2f,
                    0.6f,
                    0.2f
                });

            samples.push_back(
                BarycentricSample{
                    0.2f,
                    0.2f,
                    0.6f
                });

            if (quality_ == ThinWallQuality::Balanced) {
                return samples;
            }

            samples.push_back(
                BarycentricSample{
                    0.8f,
                    0.1f,
                    0.1f
                });

            samples.push_back(
                BarycentricSample{
                    0.1f,
                    0.8f,
                    0.1f
                });

            samples.push_back(
                BarycentricSample{
                    0.1f,
                    0.1f,
                    0.8f
                });

            return samples;
        }

        /**
         * @brief Computes confidence from quality and opposite-normal
         * alignment.
         */
        [[nodiscard]] double measurement_confidence(
            const glm::vec3& sourceDirection,
            const glm::vec3& oppositeNormal) const
        {
            const glm::vec3 source =
                normalized_or_zero(
                    sourceDirection);

            const glm::vec3 opposite =
                normalized_or_zero(
                    oppositeNormal);

            const double alignment =
                glm::clamp(
                    static_cast<double>(
                        glm::dot(
                            source,
                            opposite)),
                    0.0,
                    1.0);

            double qualityWeight = 0.65;

            switch (quality_) {
            case ThinWallQuality::Fast:
                qualityWeight = 0.65;
                break;

            case ThinWallQuality::Balanced:
                qualityWeight = 0.82;
                break;

            case ThinWallQuality::High:
                qualityWeight = 0.95;
                break;
            }

            return
                glm::clamp(
                    alignment *
                    qualityWeight,
                    0.0,
                    1.0);
        }

        /**
         * @brief Returns the process wall-thickness limit.
         */
        [[nodiscard]] static double minimum_wall_thickness(
            const PrintProfile& profile)
        {
            const auto& limit =
                profile.limits()
                .minimumWallThickness;

            if (!limit.has_value()) {
                return
                    std::numeric_limits<double>::quiet_NaN();
            }

            return limit.value();
        }

        /**
         * @brief Computes an offset that scales with the configured limit while
         * retaining a useful absolute floor.
         */
        [[nodiscard]] static float ray_origin_offset(
            double limit) noexcept
        {
            return static_cast<float>(
                std::max(
                    1.0e-6,
                    limit * 1.0e-5));
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
         * @brief Safely normalizes a vector.
         */
        [[nodiscard]] static glm::vec3 normalized_or_zero(
            const glm::vec3& value) noexcept
        {
            const float length =
                glm::length(value);

            if (!std::isfinite(length) ||
                length <= 0.0f) {
                return glm::vec3{ 0.0f };
            }

            return value / length;
        }

        /**
         * @brief Checks whether a value occurs in a vector.
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
         * @brief Adds a value when it is not already present.
         */
        template <typename T>
        static void add_unique(
            std::vector<T>& values,
            const T& value)
        {
            if (!contains(
                values,
                value)) {

                values.push_back(value);
            }
        }

        ThinWallQuality quality_ =
            ThinWallQuality::Balanced;
    };

}