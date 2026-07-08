/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/snapping/EdgeSnapProvider.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "kernel/common/Id.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/mesh/elements/Edge.h"
#include "kernel/geometry/mesh/elements/Vertex.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <limits>
#include <vector>

namespace locus::editor {
    namespace {

        struct EdgeSnapCandidate {
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

        [[nodiscard]] glm::vec3 closest_point_on_segment(
            const glm::vec3& point,
            const glm::vec3& segmentA,
            const glm::vec3& segmentB)
        {
            const glm::vec3 segment = segmentB - segmentA;
            const float lengthSquared = glm::dot(segment, segment);

            if (lengthSquared <= 0.000001f) {
                return segmentA;
            }

            const float t = glm::dot(point - segmentA, segment) / lengthSquared;
            const float clampedT = std::max(0.0f, std::min(1.0f, t));

            return segmentA + segment * clampedT;
        }

        [[nodiscard]] bool is_better_candidate(
            const EdgeSnapCandidate& current,
            const EdgeSnapCandidate& best)
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

        [[nodiscard]] EdgeSnapCandidate find_best_edge_on_node(
            const MeshNode& node,
            const SnapContext& context)
        {
            EdgeSnapCandidate best{};

            const kernel::geometry::LEM& mesh = node.mesh();
            const glm::mat4 nodeMatrix = node.transform().matrix();

            const std::vector<kernel::geometry::Edge>& edges = mesh.edges();

            for (std::size_t index = 0; index < edges.size(); ++index) {
                const kernel::geometry::EdgeHandle handle{
                    static_cast<kernel::IdValue>(index)
                };

                if (!mesh.is_valid(handle)) {
                    continue;
                }

                const kernel::geometry::Edge& edge = edges[index];

                if (edge.hidden || edge.deleted) {
                    continue;
                }

                if (!is_vertex_readable(mesh, edge.vertexA)
                    || !is_vertex_readable(mesh, edge.vertexB)) {
                    continue;
                }

                const kernel::geometry::Vertex& vertexA = mesh.vertex(edge.vertexA);
                const kernel::geometry::Vertex& vertexB = mesh.vertex(edge.vertexB);

                const glm::vec3 worldA = transform_point(nodeMatrix, vertexA.position);
                const glm::vec3 worldB = transform_point(nodeMatrix, vertexB.position);
                const glm::vec3 snapped = closest_point_on_segment(
                    context.candidatePosition,
                    worldA,
                    worldB);

                const float distance = glm::length(snapped - context.candidatePosition);

                glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
                const glm::vec3 direction = worldB - worldA;
                if (glm::length(direction) > 0.000001f) {
                    normal = glm::normalize(direction);
                }

                EdgeSnapCandidate candidate{};
                candidate.valid = true;
                candidate.node = node.id();
                candidate.component = static_cast<std::uint64_t>(index);
                candidate.worldPosition = snapped;
                candidate.worldNormal = normal;
                candidate.distance = distance;

                if (is_better_candidate(candidate, best)) {
                    best = candidate;
                }
            }

            return best;
        }

    } // namespace

    SnapMode EdgeSnapProvider::mode() const
    {
        return SnapMode::Edge;
    }

    bool EdgeSnapProvider::is_enabled(
        const SnapSettings& settings,
        const SnapContext& context) const
    {
        return settings.is_enabled(SnapMode::Edge) && context.scene != nullptr;
    }

    SnapResult EdgeSnapProvider::snap(
        const SnapSettings& settings,
        const SnapContext& context) const
    {
        (void)settings;

        if (context.scene == nullptr) {
            return SnapResult::none();
        }

        EdgeSnapCandidate best{};

        const std::vector<SceneNodeId> nodeIds = context.scene->tree().node_ids();

        for (SceneNodeId nodeId : nodeIds) {
            const MeshNode* node = context.scene->find_mesh(nodeId);

            if (node == nullptr) {
                continue;
            }

            if (!node->is_visible() || !node->is_selectable()) {
                continue;
            }

            const EdgeSnapCandidate candidate = find_best_edge_on_node(*node, context);

            if (is_better_candidate(candidate, best)) {
                best = candidate;
            }
        }

        if (!best.valid) {
            return SnapResult::none();
        }

        SnapTarget target{};
        target.type = SnapTargetType::Edge;
        target.position = best.worldPosition;
        target.normal = best.worldNormal;
        target.node = best.node;
        target.component = best.component;

        return SnapResult::make(
            SnapMode::Edge,
            target,
            context.originalPosition,
            context.candidatePosition,
            best.worldPosition,
            best.distance,
            best.distance);
    }

} // namespace locus::editor