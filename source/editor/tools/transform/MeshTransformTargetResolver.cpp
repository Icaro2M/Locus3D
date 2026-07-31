/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/transform/MeshTransformTargetResolver.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"
#include "editor/selection/MeshSelection.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/selection/SelectionState.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        using kernel::geometry::EdgeHandle;
        using kernel::geometry::FaceHandle;
        using kernel::geometry::LEM;
        using kernel::geometry::TopologyTraversal;
        using kernel::geometry::VertexHandle;

        [[nodiscard]] glm::mat4 node_world_matrix(
            const EditorScene& scene,
            SceneNodeId node)
        {
            const SceneNode* sceneNode = scene.find_node(node);
            if (!sceneNode) {
                return glm::mat4{ 1.0f };
            }

            const glm::mat4 local = sceneNode->transform().matrix();
            if (sceneNode->parent().is_invalid()) {
                return local;
            }

            return node_world_matrix(scene, sceneNode->parent()) * local;
        }

        void add_unique_vertex(
            std::vector<VertexHandle>& vertices,
            VertexHandle vertex)
        {
            if (vertex.is_invalid()) {
                return;
            }

            if (std::find(vertices.begin(), vertices.end(), vertex)
                == vertices.end()) {
                vertices.push_back(vertex);
            }
        }

        [[nodiscard]] bool validate_vertices(
            const LEM& mesh,
            const std::vector<VertexHandle>& vertices)
        {
            for (VertexHandle vertex : vertices) {
                if (!mesh.is_valid(vertex)) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] glm::vec3 world_position(
            const glm::mat4& world,
            const LEM& mesh,
            VertexHandle vertex)
        {
            return glm::vec3{
                world * glm::vec4{ mesh.vertex(vertex).position, 1.0f }
            };
        }

        [[nodiscard]] glm::vec3 geometric_center(
            const glm::mat4& world,
            const LEM& mesh,
            const std::vector<VertexHandle>& vertices)
        {
            glm::vec3 sum{ 0.0f, 0.0f, 0.0f };

            for (VertexHandle vertex : vertices) {
                sum += world_position(world, mesh, vertex);
            }

            return sum / static_cast<float>(vertices.size());
        }

        [[nodiscard]] std::vector<VertexHandle> resolve_vertices(
            const LEM& mesh,
            const MeshSelection& selection)
        {
            std::vector<VertexHandle> vertices{};
            const std::vector<VertexHandle> selected =
                selection.vertices().items();

            vertices.reserve(selected.size());

            for (VertexHandle vertex : selected) {
                if (!mesh.is_valid(vertex)) {
                    return {};
                }

                add_unique_vertex(vertices, vertex);
            }

            return vertices;
        }

        [[nodiscard]] std::vector<VertexHandle> resolve_edges(
            const LEM& mesh,
            const MeshSelection& selection)
        {
            std::vector<VertexHandle> vertices{};
            const std::vector<EdgeHandle> selected =
                selection.edges().items();

            vertices.reserve(selected.size() * 2U);

            for (EdgeHandle edge : selected) {
                if (!mesh.is_valid(edge)) {
                    return {};
                }

                const auto endpoints =
                    TopologyTraversal::edge_vertices(mesh, edge);

                if (!mesh.is_valid(endpoints[0]) ||
                    !mesh.is_valid(endpoints[1])) {
                    return {};
                }

                add_unique_vertex(vertices, endpoints[0]);
                add_unique_vertex(vertices, endpoints[1]);
            }

            return vertices;
        }

        [[nodiscard]] std::vector<VertexHandle> resolve_faces(
            const LEM& mesh,
            const MeshSelection& selection)
        {
            std::vector<VertexHandle> vertices{};
            const std::vector<FaceHandle> selected =
                selection.faces().items();

            for (FaceHandle face : selected) {
                if (!mesh.is_valid(face)) {
                    return {};
                }

                const std::vector<VertexHandle> faceVertices =
                    TopologyTraversal::face_vertices(mesh, face);

                if (faceVertices.empty()) {
                    return {};
                }

                for (VertexHandle vertex : faceVertices) {
                    if (!mesh.is_valid(vertex)) {
                        return {};
                    }

                    add_unique_vertex(vertices, vertex);
                }
            }

            return vertices;
        }

    } // namespace

    MeshTransformTargetResolveResult
        MeshTransformTargetResolveResult::ok(
            MeshTransformTarget resolved)
    {
        MeshTransformTargetResolveResult result{};
        result.success = true;
        result.target = std::move(resolved);
        return result;
    }

    MeshTransformTargetResolveResult
        MeshTransformTargetResolveResult::fail(
            std::string message)
    {
        MeshTransformTargetResolveResult result{};
        result.message = std::move(message);
        return result;
    }

    MeshTransformTargetResolveResult MeshTransformTargetResolver::resolve(
        const EditorScene& scene,
        const SelectionState& selection)
    {
        if (selection.scope() != SelectionScope::ActiveMesh) {
            return MeshTransformTargetResolveResult::fail(
                "Mesh transforms require active mesh selection scope.");
        }

        const SelectionGranularity granularity =
            selection.granularity();

        if (granularity != SelectionGranularity::Vertex &&
            granularity != SelectionGranularity::Edge &&
            granularity != SelectionGranularity::Face) {
            return MeshTransformTargetResolveResult::fail(
                "Mesh transforms require vertex, edge, or face granularity.");
        }

        const MeshSelection& meshSelection = selection.mesh();
        const SceneNodeId activeMesh = meshSelection.active_mesh();
        if (activeMesh.is_invalid()) {
            return MeshTransformTargetResolveResult::fail(
                "Mesh transforms require a valid active mesh.");
        }

        const MeshNode* node = scene.find_mesh(activeMesh);
        if (!node) {
            return MeshTransformTargetResolveResult::fail(
                "Active mesh node was not found.");
        }

        const LEM& mesh = node->mesh();
        std::vector<VertexHandle> vertices{};

        switch (granularity) {
        case SelectionGranularity::Vertex:
            if (meshSelection.vertices().empty()) {
                return MeshTransformTargetResolveResult::fail(
                    "Cannot transform an empty vertex selection.");
            }
            vertices = resolve_vertices(mesh, meshSelection);
            break;
        case SelectionGranularity::Edge:
            if (meshSelection.edges().empty()) {
                return MeshTransformTargetResolveResult::fail(
                    "Cannot transform an empty edge selection.");
            }
            vertices = resolve_edges(mesh, meshSelection);
            break;
        case SelectionGranularity::Face:
            if (meshSelection.faces().empty()) {
                return MeshTransformTargetResolveResult::fail(
                    "Cannot transform an empty face selection.");
            }
            vertices = resolve_faces(mesh, meshSelection);
            break;
        case SelectionGranularity::Loop:
        case SelectionGranularity::Object:
        default:
            return MeshTransformTargetResolveResult::fail(
                "Unsupported mesh transform granularity.");
        }

        if (vertices.empty() || !validate_vertices(mesh, vertices)) {
            return MeshTransformTargetResolveResult::fail(
                "Mesh transform selection resolved no valid vertices.");
        }

        MeshTransformTarget target{};
        target.node = activeMesh;
        target.vertices = std::move(vertices);
        target.pivot = geometric_center(
            node_world_matrix(scene, activeMesh),
            mesh,
            target.vertices);

        return MeshTransformTargetResolveResult::ok(std::move(target));
    }

} // namespace locus::editor
