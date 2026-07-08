/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/snapping/FaceSnapProvider.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "kernel/common/Id.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/mesh/elements/Face.h"
#include "kernel/geometry/mesh/elements/Loop.h"
#include "kernel/geometry/mesh/elements/Vertex.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <limits>
#include <vector>

namespace locus::editor {
    namespace {

        struct FaceSnapCandidate {
            bool valid = false;
            SceneNodeId node{};
            std::uint64_t component = std::numeric_limits<std::uint64_t>::max();
            glm::vec3 worldPosition{ 0.0f, 0.0f, 0.0f };
            glm::vec3 worldNormal{ 0.0f, 1.0f, 0.0f };
            float distance = 0.0f;
        };

        [[nodiscard]] glm::vec3 transform_point(
            const glm::mat4& transform,
            const glm::vec3& point)
        {
            return glm::vec3{ transform * glm::vec4{ point, 1.0f } };
        }

        [[nodiscard]] bool is_better_candidate(
            const FaceSnapCandidate& current,
            const FaceSnapCandidate& best)
        {
            if (!current.valid) {
                return false;
            }

            if (!best.valid) {
                return true;
            }

            return current.distance < best.distance;
        }

        [[nodiscard]] bool is_vertex_readable(
            const kernel::geometry::LEM& mesh,
            kernel::geometry::VertexHandle handle)
        {
            if (!mesh.is_valid(handle)) {
                return false;
            }

            const kernel::geometry::Vertex& vertex = mesh.vertex(handle);
            return !vertex.hidden && !vertex.deleted;
        }

        [[nodiscard]] glm::vec3 compute_world_polygon_normal(
            const std::vector<glm::vec3>& points)
        {
            glm::vec3 normal{ 0.0f, 0.0f, 0.0f };

            if (points.size() < 3u) {
                return { 0.0f, 1.0f, 0.0f };
            }

            for (std::size_t index = 0; index < points.size(); ++index) {
                const glm::vec3& current = points[index];
                const glm::vec3& next = points[(index + 1u) % points.size()];

                normal.x += (current.y - next.y) * (current.z + next.z);
                normal.y += (current.z - next.z) * (current.x + next.x);
                normal.z += (current.x - next.x) * (current.y + next.y);
            }

            const float length = glm::length(normal);
            if (length <= 0.000001f) {
                return { 0.0f, 1.0f, 0.0f };
            }

            return normal / length;
        }

        [[nodiscard]] glm::vec3 project_point_on_plane(
            const glm::vec3& point,
            const glm::vec3& planePoint,
            const glm::vec3& planeNormal)
        {
            const float signedDistance = glm::dot(point - planePoint, planeNormal);
            return point - planeNormal * signedDistance;
        }

        [[nodiscard]] int dominant_axis(const glm::vec3& normal)
        {
            const glm::vec3 absNormal{
                std::abs(normal.x),
                std::abs(normal.y),
                std::abs(normal.z)
            };

            if (absNormal.x >= absNormal.y && absNormal.x >= absNormal.z) {
                return 0;
            }

            if (absNormal.y >= absNormal.x && absNormal.y >= absNormal.z) {
                return 1;
            }

            return 2;
        }

        [[nodiscard]] glm::vec2 project_to_2d(const glm::vec3& point, int axisToDrop)
        {
            switch (axisToDrop) {
            case 0:
                return { point.y, point.z };
            case 1:
                return { point.x, point.z };
            default:
                return { point.x, point.y };
            }
        }

        [[nodiscard]] bool point_on_segment_2d(
            const glm::vec2& point,
            const glm::vec2& segmentA,
            const glm::vec2& segmentB)
        {
            constexpr float epsilon = 0.00001f;

            const glm::vec2 ab = segmentB - segmentA;
            const glm::vec2 ap = point - segmentA;

            const float cross = ab.x * ap.y - ab.y * ap.x;
            if (std::abs(cross) > epsilon) {
                return false;
            }

            const float dot = glm::dot(ap, ab);
            if (dot < -epsilon) {
                return false;
            }

            const float lengthSquared = glm::dot(ab, ab);
            if (dot > lengthSquared + epsilon) {
                return false;
            }

            return true;
        }

        [[nodiscard]] bool point_inside_polygon_2d(
            const glm::vec2& point,
            const std::vector<glm::vec2>& polygon)
        {
            if (polygon.size() < 3u) {
                return false;
            }

            bool inside = false;

            for (std::size_t i = 0u, j = polygon.size() - 1u; i < polygon.size(); j = i++) {
                const glm::vec2& a = polygon[i];
                const glm::vec2& b = polygon[j];

                if (point_on_segment_2d(point, a, b)) {
                    return true;
                }

                const bool crosses = ((a.y > point.y) != (b.y > point.y))
                    && (point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x);

                if (crosses) {
                    inside = !inside;
                }
            }

            return inside;
        }

        [[nodiscard]] bool point_inside_world_polygon(
            const glm::vec3& point,
            const std::vector<glm::vec3>& polygon,
            const glm::vec3& normal)
        {
            const int axisToDrop = dominant_axis(normal);

            std::vector<glm::vec2> projectedPolygon;
            projectedPolygon.reserve(polygon.size());

            for (const glm::vec3& vertex : polygon) {
                projectedPolygon.push_back(project_to_2d(vertex, axisToDrop));
            }

            const glm::vec2 projectedPoint = project_to_2d(point, axisToDrop);
            return point_inside_polygon_2d(projectedPoint, projectedPolygon);
        }

        [[nodiscard]] bool collect_face_world_points(
            const kernel::geometry::LEM& mesh,
            kernel::geometry::FaceHandle faceHandle,
            const glm::mat4& nodeMatrix,
            std::vector<glm::vec3>& points)
        {
            points.clear();

            const std::vector<kernel::geometry::LoopHandle> loops = mesh.face_loops(faceHandle);
            if (loops.size() < 3u) {
                return false;
            }

            points.reserve(loops.size());

            for (kernel::geometry::LoopHandle loopHandle : loops) {
                if (!mesh.is_valid(loopHandle)) {
                    return false;
                }

                const kernel::geometry::Loop& loop = mesh.loop(loopHandle);

                if (!is_vertex_readable(mesh, loop.vertex)) {
                    return false;
                }

                const kernel::geometry::Vertex& vertex = mesh.vertex(loop.vertex);
                points.push_back(transform_point(nodeMatrix, vertex.position));
            }

            return points.size() >= 3u;
        }

        [[nodiscard]] FaceSnapCandidate find_best_face_on_node(
            const MeshNode& node,
            const SnapContext& context)
        {
            FaceSnapCandidate best{};

            const kernel::geometry::LEM& mesh = node.mesh();
            const glm::mat4 nodeMatrix = node.transform().matrix();
            const std::vector<kernel::geometry::Face>& faces = mesh.faces();

            std::vector<glm::vec3> facePoints;

            for (std::size_t index = 0; index < faces.size(); ++index) {
                const kernel::geometry::FaceHandle handle{
                    static_cast<kernel::IdValue>(index)
                };

                if (!mesh.is_valid(handle)) {
                    continue;
                }

                const kernel::geometry::Face& face = faces[index];

                if (face.hidden || face.deleted) {
                    continue;
                }

                if (!collect_face_world_points(mesh, handle, nodeMatrix, facePoints)) {
                    continue;
                }

                const glm::vec3 normal = compute_world_polygon_normal(facePoints);
                const glm::vec3 projected = project_point_on_plane(
                    context.candidatePosition,
                    facePoints.front(),
                    normal);

                if (!point_inside_world_polygon(projected, facePoints, normal)) {
                    continue;
                }

                const float distance = glm::length(projected - context.candidatePosition);

                FaceSnapCandidate candidate{};
                candidate.valid = true;
                candidate.node = node.id();
                candidate.component = static_cast<std::uint64_t>(index);
                candidate.worldPosition = projected;
                candidate.worldNormal = normal;
                candidate.distance = distance;

                if (is_better_candidate(candidate, best)) {
                    best = candidate;
                }
            }

            return best;
        }

    } // namespace

    SnapMode FaceSnapProvider::mode() const
    {
        return SnapMode::Face;
    }

    bool FaceSnapProvider::is_enabled(
        const SnapSettings& settings,
        const SnapContext& context) const
    {
        return settings.is_enabled(SnapMode::Face) && context.scene != nullptr;
    }

    SnapResult FaceSnapProvider::snap(
        const SnapSettings& settings,
        const SnapContext& context) const
    {
        (void)settings;

        if (context.scene == nullptr) {
            return SnapResult::none();
        }

        FaceSnapCandidate best{};

        const std::vector<SceneNodeId> nodeIds = context.scene->tree().node_ids();

        for (SceneNodeId nodeId : nodeIds) {
            const MeshNode* node = context.scene->find_mesh(nodeId);

            if (node == nullptr) {
                continue;
            }

            if (!node->is_visible() || !node->is_selectable()) {
                continue;
            }

            const FaceSnapCandidate candidate = find_best_face_on_node(*node, context);

            if (is_better_candidate(candidate, best)) {
                best = candidate;
            }
        }

        if (!best.valid) {
            return SnapResult::none();
        }

        SnapTarget target{};
        target.type = SnapTargetType::Face;
        target.position = best.worldPosition;
        target.normal = best.worldNormal;
        target.node = best.node;
        target.component = best.component;

        return SnapResult::make(
            SnapMode::Face,
            target,
            context.originalPosition,
            context.candidatePosition,
            best.worldPosition,
            best.distance,
            best.distance);
    }

} // namespace locus::editor