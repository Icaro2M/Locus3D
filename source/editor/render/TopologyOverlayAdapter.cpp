/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/TopologyOverlayAdapter.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"
#include "editor/selection/SelectionState.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/vec4.hpp>

#include <vector>

namespace locus::editor {

    namespace {

        using locus::kernel::geometry::Edge;
        using locus::kernel::geometry::EdgeHandle;
        using locus::kernel::geometry::LEM;
        using locus::kernel::geometry::TopologyTraversal;
        using locus::kernel::geometry::Vertex;

        enum class EdgeVisualState {
            Wireframe,
            Hovered,
            Selected
        };

        [[nodiscard]] glm::mat4 node_world_matrix(
            const EditorScene& scene,
            SceneNodeId node)
        {
            const SceneNode* sceneNode = scene.find_node(node);
            if (sceneNode == nullptr) {
                return glm::mat4{ 1.0f };
            }

            const glm::mat4 local = sceneNode->transform().matrix();
            if (sceneNode->parent().is_invalid()) {
                return local;
            }

            return node_world_matrix(scene, sceneNode->parent()) * local;
        }

        [[nodiscard]] glm::vec3 transform_point(
            const glm::mat4& matrix,
            const glm::vec3& point)
        {
            return glm::vec3{ matrix * glm::vec4{ point, 1.0f } };
        }

        [[nodiscard]] bool should_skip_edge(
            const LEM& mesh,
            EdgeHandle handle,
            bool skipHidden)
        {
            if (!mesh.is_valid(handle)) {
                return true;
            }

            return skipHidden && mesh.edge(handle).hidden;
        }

        [[nodiscard]] bool can_apply_edge_selection(
            const SelectionState& selection,
            SceneNodeId nodeId)
        {
            return selection.scope() == SelectionScope::ActiveMesh
                && selection.granularity() == SelectionGranularity::Edge
                && selection.mesh().active_mesh() == nodeId;
        }

        [[nodiscard]] EdgeVisualState edge_visual_state(
            EdgeHandle handle,
            const SelectionState& selection,
            SceneNodeId nodeId)
        {
            if (!can_apply_edge_selection(selection, nodeId)) {
                return EdgeVisualState::Wireframe;
            }

            if (selection.mesh().edges().contains(handle)) {
                return EdgeVisualState::Selected;
            }

            if (selection.mesh().hovered_edge() == handle) {
                return EdgeVisualState::Hovered;
            }

            return EdgeVisualState::Wireframe;
        }

        [[nodiscard]] graphics::ScreenSpaceLine make_line(
            const glm::vec3& start,
            const glm::vec3& end,
            EdgeVisualState state,
            const graphics::TopologyOverlayStyle& style)
        {
            graphics::ScreenSpaceLine line{};
            line.start = start;
            line.end = end;

            switch (state) {
            case EdgeVisualState::Selected:
                line.color = style.selectedEdgeColor;
                line.widthPixels = style.selectedEdgeWidthPixels;
                break;

            case EdgeVisualState::Hovered:
                line.color = style.hoveredEdgeColor;
                line.widthPixels = style.hoveredEdgeWidthPixels;
                break;

            case EdgeVisualState::Wireframe:
                line.color = style.wireframeColor;
                line.widthPixels = style.wireframeWidthPixels;
                break;
            }

            return line;
        }

        void count_state(EdgeVisualState state, TopologyOverlayResult* result)
        {
            if (result == nullptr) {
                return;
            }

            switch (state) {
            case EdgeVisualState::Selected:
                ++result->selectedEdgeCount;
                break;
            case EdgeVisualState::Hovered:
                ++result->hoveredEdgeCount;
                break;
            case EdgeVisualState::Wireframe:
                ++result->wireframeEdgeCount;
                break;
            }
        }

        void count_invalid_selected_edges(
            const LEM& mesh,
            const SelectionState& selection,
            SceneNodeId nodeId,
            TopologyOverlayResult* result)
        {
            if (result == nullptr || !can_apply_edge_selection(selection, nodeId)) {
                return;
            }

            for (const EdgeHandle handle : selection.mesh().edges().items()) {
                if (!mesh.is_valid(handle)) {
                    ++result->invalidHandleCount;
                }
            }

            const EdgeHandle hovered = selection.mesh().hovered_edge();
            if (hovered.is_valid() && !mesh.is_valid(hovered)) {
                ++result->invalidHandleCount;
            }
        }

    } // namespace

    graphics::ScreenSpaceLineBatch TopologyOverlayAdapter::build_active_mesh_lines(
        const EditorScene& scene,
        const SelectionState& selection,
        const TopologyOverlayOptions& options,
        TopologyOverlayResult* result)
    {
        if (result != nullptr) {
            *result = {};
            result->nodeId = selection.mesh().active_mesh();
        }

        graphics::ScreenSpaceLineBatch batch{};

        const SceneNodeId activeMesh = selection.mesh().active_mesh();
        if (!activeMesh.is_valid()) {
            if (result != nullptr) {
                result->message = "No active mesh available for topology overlay.";
            }
            return batch;
        }

        const MeshNode* meshNode = scene.find_mesh(activeMesh);
        if (meshNode == nullptr) {
            if (result != nullptr) {
                result->message = "Active mesh id did not resolve to a mesh node.";
            }
            return batch;
        }

        if (!meshNode->is_visible()) {
            if (result != nullptr) {
                result->message = "Active mesh is hidden.";
            }
            return batch;
        }

        const LEM& mesh = meshNode->mesh();
        if (mesh.empty()) {
            if (result != nullptr) {
                result->message = "Active mesh is empty.";
            }
            return batch;
        }

        const std::vector<EdgeHandle> edges = TopologyTraversal::edges(mesh);
        const glm::mat4 world = node_world_matrix(scene, activeMesh);
        batch.lines.reserve(edges.size());

        if (result != nullptr) {
            result->visitedEdgeCount = edges.size();
        }

        count_invalid_selected_edges(mesh, selection, activeMesh, result);

        for (const EdgeHandle handle : edges) {
            if (should_skip_edge(mesh, handle, options.skipHiddenComponents)) {
                continue;
            }

            const Edge& edge = mesh.edge(handle);
            if (!mesh.is_valid(edge.vertexA) || !mesh.is_valid(edge.vertexB)) {
                if (result != nullptr) {
                    ++result->invalidHandleCount;
                }
                continue;
            }

            const Vertex& vertexA = mesh.vertex(edge.vertexA);
            const Vertex& vertexB = mesh.vertex(edge.vertexB);
            const EdgeVisualState state = edge_visual_state(
                handle,
                selection,
                activeMesh);

            batch.lines.push_back(
                make_line(
                    transform_point(world, vertexA.position),
                    transform_point(world, vertexB.position),
                    state,
                    options.style));

            count_state(state, result);
        }

        if (result != nullptr) {
            result->message = result->has_geometry()
                ? "Topology overlay lines built successfully."
                : "Active mesh produced no drawable topology overlay lines.";
        }

        return batch;
    }

} // namespace locus::editor
