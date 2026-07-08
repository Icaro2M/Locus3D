/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/snapping/VertexSnapProvider.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "kernel/geometry.h"


#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <limits>
#include <vector>

namespace locus::editor {
    namespace {

        struct VertexSnapCandidate {
            bool valid = false;
            SceneNodeId node{};
            std::uint64_t component = std::numeric_limits<std::uint64_t>::max();
            glm::vec3 worldPosition{ 0.0f, 0.0f, 0.0f };
            float distance = 0.0f;
        };

        [[nodiscard]] glm::vec3 transform_point(
            const glm::mat4& transform,
            const glm::vec3& point)
        {
            return glm::vec3{ transform * glm::vec4{ point, 1.0f } };
        }

        [[nodiscard]] bool is_better_candidate(
            const VertexSnapCandidate& current,
            const VertexSnapCandidate& best)
        {
            if (!current.valid) {
                return false;
            }

            if (!best.valid) {
                return true;
            }

            return current.distance < best.distance;
        }

        [[nodiscard]] VertexSnapCandidate find_best_vertex_on_node(
            const MeshNode& node,
            const SnapContext& context)
        {
            VertexSnapCandidate best{};

            const kernel::geometry::LEM& mesh = node.mesh();
            const glm::mat4 nodeMatrix = node.transform().matrix();

            const std::vector<kernel::geometry::Vertex>& vertices = mesh.vertices();

            for (std::size_t index = 0; index < vertices.size(); ++index) {
                
                const kernel::geometry::VertexHandle handle{
                    static_cast<kernel::IdValue>(index)
                };

                if (!mesh.is_valid(handle)) {
                    continue;
                }

                const kernel::geometry::Vertex& vertex = vertices[index];

                if (vertex.hidden || vertex.deleted) {
                    continue;
                }

                const glm::vec3 worldPosition = transform_point(nodeMatrix, vertex.position);
                const float distance = glm::length(worldPosition - context.candidatePosition);

                VertexSnapCandidate candidate{};
                candidate.valid = true;
                candidate.node = node.id();
                candidate.component = static_cast<std::uint64_t>(index);
                candidate.worldPosition = worldPosition;
                candidate.distance = distance;

                if (is_better_candidate(candidate, best)) {
                    best = candidate;
                }
            }

            return best;
        }

    } // namespace

    SnapMode VertexSnapProvider::mode() const
    {
        return SnapMode::Vertex;
    }

    bool VertexSnapProvider::is_enabled(
        const SnapSettings& settings,
        const SnapContext& context) const
    {
        return settings.is_enabled(SnapMode::Vertex) && context.scene != nullptr;
    }

    SnapResult VertexSnapProvider::snap(
        const SnapSettings& settings,
        const SnapContext& context) const
    {
        (void)settings;

        if (context.scene == nullptr) {
            return SnapResult::none();
        }

        VertexSnapCandidate best{};

        const std::vector<SceneNodeId> nodeIds = context.scene->tree().node_ids();

        for (SceneNodeId nodeId : nodeIds) {
            const MeshNode* node = context.scene->find_mesh(nodeId);

            if (node == nullptr) {
                continue;
            }

            if (!node->is_visible() || !node->is_selectable()) {
                continue;
            }

            const VertexSnapCandidate candidate = find_best_vertex_on_node(*node, context);

            if (is_better_candidate(candidate, best)) {
                best = candidate;
            }
        }

        if (!best.valid) {
            return SnapResult::none();
        }

        SnapTarget target{};
        target.type = SnapTargetType::Vertex;
        target.position = best.worldPosition;
        target.node = best.node;
        target.component = best.component;

        return SnapResult::make(
            SnapMode::Vertex,
            target,
            context.originalPosition,
            context.candidatePosition,
            best.worldPosition,
            best.distance,
            best.distance);
    }

} // namespace locus::editor