/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/TopologyOverlayAdapter.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"
#include "editor/scene/SceneTransforms.h"
#include "editor/selection/SelectionState.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/vec4.hpp>

#include <cstdint>
#include <vector>

namespace locus::editor {

    namespace {

        using locus::kernel::geometry::Edge;
        using locus::kernel::geometry::EdgeHandle;
        using locus::kernel::geometry::FaceHandle;
        using locus::kernel::geometry::LEM;
        using locus::kernel::geometry::MeshTriangulator;
        using locus::kernel::geometry::RenderMesh;
        using locus::kernel::geometry::RenderTriangle;
        using locus::kernel::geometry::RenderVertex;
        using locus::kernel::geometry::TopologyTraversal;
        using locus::kernel::geometry::Vertex;
        using locus::kernel::geometry::VertexHandle;

        enum class EdgeVisualState {
            Wireframe,
            Hovered,
            Selected
        };

        enum class VertexVisualState {
            Normal,
            Hovered,
            Selected
        };

        enum class FaceVisualState {
            Normal,
            Hovered,
            Selected
        };

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

        [[nodiscard]] bool should_skip_vertex(
            const LEM& mesh,
            VertexHandle handle,
            bool skipHidden)
        {
            if (!mesh.is_valid(handle)) {
                return true;
            }

            return skipHidden && mesh.vertex(handle).hidden;
        }

        [[nodiscard]] bool should_skip_face(
            const LEM& mesh,
            FaceHandle handle,
            bool skipHidden)
        {
            if (!mesh.is_valid(handle)) {
                return true;
            }

            return skipHidden && mesh.face(handle).hidden;
        }

        [[nodiscard]] bool can_apply_edge_selection(
            const SelectionState& selection,
            SceneNodeId nodeId)
        {
            return selection.scope() == SelectionScope::ActiveMesh
                && selection.granularity() == SelectionGranularity::Edge
                && selection.mesh().active_mesh() == nodeId;
        }

        [[nodiscard]] bool can_apply_vertex_selection(
            const SelectionState& selection,
            SceneNodeId nodeId)
        {
            return selection.scope() == SelectionScope::ActiveMesh
                && selection.granularity() == SelectionGranularity::Vertex
                && selection.mesh().active_mesh() == nodeId;
        }

        [[nodiscard]] bool can_apply_face_selection(
            const SelectionState& selection,
            SceneNodeId nodeId)
        {
            return selection.scope() == SelectionScope::ActiveMesh
                && selection.granularity() == SelectionGranularity::Face
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

        [[nodiscard]] VertexVisualState vertex_visual_state(
            VertexHandle handle,
            const SelectionState& selection,
            SceneNodeId nodeId)
        {
            if (!can_apply_vertex_selection(selection, nodeId)) {
                return VertexVisualState::Normal;
            }

            if (selection.mesh().vertices().contains(handle)) {
                return VertexVisualState::Selected;
            }

            if (selection.mesh().hovered_vertex() == handle) {
                return VertexVisualState::Hovered;
            }

            return VertexVisualState::Normal;
        }

        [[nodiscard]] FaceVisualState face_visual_state(
            FaceHandle handle,
            const SelectionState& selection,
            SceneNodeId nodeId)
        {
            if (!can_apply_face_selection(selection, nodeId)) {
                return FaceVisualState::Normal;
            }

            if (selection.mesh().faces().contains(handle)) {
                return FaceVisualState::Selected;
            }

            if (selection.mesh().hovered_face() == handle) {
                return FaceVisualState::Hovered;
            }

            return FaceVisualState::Normal;
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

        [[nodiscard]] graphics::PointMarker make_marker(
            const glm::vec3& position,
            VertexVisualState state,
            const graphics::TopologyOverlayStyle& style)
        {
            graphics::PointMarker marker{};
            marker.position = position;
            marker.borderColor = style.vertexBorderColor;
            marker.borderWidthPixels = style.vertexBorderWidthPixels;

            switch (state) {
            case VertexVisualState::Selected:
                marker.fillColor = style.selectedVertexColor;
                marker.radiusPixels = style.selectedVertexRadiusPixels;
                break;

            case VertexVisualState::Hovered:
                marker.fillColor = style.hoveredVertexColor;
                marker.radiusPixels = style.hoveredVertexRadiusPixels;
                break;

            case VertexVisualState::Normal:
                marker.fillColor = style.vertexColor;
                marker.radiusPixels = style.vertexRadiusPixels;
                break;
            }

            return marker;
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

        void count_state(VertexVisualState state, TopologyOverlayResult* result)
        {
            if (result == nullptr) {
                return;
            }

            switch (state) {
            case VertexVisualState::Selected:
                ++result->selectedVertexCount;
                break;
            case VertexVisualState::Hovered:
                ++result->hoveredVertexCount;
                break;
            case VertexVisualState::Normal:
                ++result->normalVertexCount;
                break;
            }
        }

        void count_state(FaceVisualState state, TopologyOverlayResult* result)
        {
            if (result == nullptr) {
                return;
            }

            switch (state) {
            case FaceVisualState::Selected:
                ++result->selectedFaceCount;
                break;
            case FaceVisualState::Hovered:
                ++result->hoveredFaceCount;
                break;
            case FaceVisualState::Normal:
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

        void count_invalid_selected_vertices(
            const LEM& mesh,
            const SelectionState& selection,
            SceneNodeId nodeId,
            TopologyOverlayResult* result)
        {
            if (result == nullptr || !can_apply_vertex_selection(selection, nodeId)) {
                return;
            }

            for (const VertexHandle handle : selection.mesh().vertices().items()) {
                if (!mesh.is_valid(handle)) {
                    ++result->invalidHandleCount;
                }
            }

            const VertexHandle hovered = selection.mesh().hovered_vertex();
            if (hovered.is_valid() && !mesh.is_valid(hovered)) {
                ++result->invalidHandleCount;
            }
        }

        void count_invalid_selected_faces(
            const LEM& mesh,
            const SelectionState& selection,
            SceneNodeId nodeId,
            TopologyOverlayResult* result)
        {
            if (result == nullptr || !can_apply_face_selection(selection, nodeId)) {
                return;
            }

            for (const FaceHandle handle : selection.mesh().faces().items()) {
                if (!mesh.is_valid(handle)) {
                    ++result->invalidHandleCount;
                }
            }

            const FaceHandle hovered = selection.mesh().hovered_face();
            if (hovered.is_valid() && !mesh.is_valid(hovered)) {
                ++result->invalidHandleCount;
            }
        }

        [[nodiscard]] glm::mat4 node_world_matrix(
            const EditorScene& scene,
            SceneNodeId node)
        {
            return SceneTransforms::world_matrix(scene, node);
        }

        void append_face_surface(
            graphics::SurfaceOverlayBatch& batch,
            const LEM& mesh,
            FaceHandle handle,
            const graphics::ColorRGBA& color,
            TopologyOverlayResult* result)
        {
            RenderMesh renderMesh{};
            MeshTriangulator::triangulate_face_into(mesh, handle, renderMesh);

            if (renderMesh.triangles.empty()) {
                return;
            }

            const std::uint32_t base =
                static_cast<std::uint32_t>(batch.vertices.size());
            batch.vertices.reserve(batch.vertices.size() + renderMesh.vertices.size());
            batch.indices.reserve(batch.indices.size() + renderMesh.triangles.size() * 3u);

            for (const RenderVertex& vertex : renderMesh.vertices) {
                graphics::SurfaceOverlayVertex overlayVertex{};
                overlayVertex.position = vertex.position;
                overlayVertex.color = color;
                batch.vertices.push_back(overlayVertex);
            }

            for (const RenderTriangle& triangle : renderMesh.triangles) {
                if (triangle.a >= renderMesh.vertices.size() ||
                    triangle.b >= renderMesh.vertices.size() ||
                    triangle.c >= renderMesh.vertices.size()) {
                    if (result != nullptr) {
                        ++result->invalidHandleCount;
                    }
                    continue;
                }

                batch.indices.push_back(base + triangle.a);
                batch.indices.push_back(base + triangle.b);
                batch.indices.push_back(base + triangle.c);

                if (result != nullptr) {
                    ++result->surfaceTriangleCount;
                }
            }
        }

        void append_mesh_lines(
            graphics::ScreenSpaceLineBatch& batch,
            const LEM& mesh,
            SceneNodeId nodeId,
            const glm::mat4& world,
            const SelectionState& selection,
            const TopologyOverlayOptions& options,
            TopologyOverlayResult* result)
        {
            const std::vector<EdgeHandle> edges = TopologyTraversal::edges(mesh);

            batch.lines.reserve(batch.lines.size() + edges.size());

            if (result != nullptr) {
                result->visitedEdgeCount += edges.size();
            }

            count_invalid_selected_edges(mesh, selection, nodeId, result);

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
                    nodeId);

                batch.lines.push_back(
                    make_line(
                        transform_point(world, vertexA.position),
                        transform_point(world, vertexB.position),
                        state,
                        options.style));

                count_state(state, result);
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

        const glm::mat4 world = node_world_matrix(scene, activeMesh);
        append_mesh_lines(
            batch,
            mesh,
            activeMesh,
            world,
            selection,
            options,
            result);

        if (result != nullptr) {
            result->message = result->has_geometry()
                ? "Topology overlay lines built successfully."
                : "Active mesh produced no drawable topology overlay lines.";
        }

        return batch;
    }

    graphics::ScreenSpaceLineBatch TopologyOverlayAdapter::build_visible_mesh_lines(
        const EditorScene& scene,
        const SelectionState& selection,
        const TopologyOverlayOptions& options,
        TopologyOverlayResult* result)
    {
        if (result != nullptr) {
            *result = {};
        }

        graphics::ScreenSpaceLineBatch batch{};

        for (const SceneNodeId nodeId : scene.tree().node_ids()) {
            const MeshNode* meshNode = scene.find_mesh(nodeId);
            if (meshNode == nullptr || !meshNode->is_visible()) {
                continue;
            }

            const LEM& mesh = meshNode->mesh();
            if (mesh.empty()) {
                continue;
            }

            append_mesh_lines(
                batch,
                mesh,
                nodeId,
                node_world_matrix(scene, nodeId),
                selection,
                options,
                result);
        }

        if (result != nullptr) {
            result->message = result->has_geometry()
                ? "Visible mesh topology lines built successfully."
                : "Scene produced no drawable visible mesh topology lines.";
        }

        return batch;
    }

    graphics::PointMarkerBatch TopologyOverlayAdapter::build_active_mesh_vertex_markers(
        const EditorScene& scene,
        const SelectionState& selection,
        const TopologyOverlayOptions& options,
        TopologyOverlayResult* result)
    {
        if (result != nullptr) {
            *result = {};
            result->nodeId = selection.mesh().active_mesh();
        }

        graphics::PointMarkerBatch batch{};

        const SceneNodeId activeMesh = selection.mesh().active_mesh();
        if (!activeMesh.is_valid()) {
            if (result != nullptr) {
                result->message = "No active mesh available for vertex overlay.";
            }
            return batch;
        }

        if (!can_apply_vertex_selection(selection, activeMesh)) {
            if (result != nullptr) {
                result->message = "Vertex overlay is disabled outside active-mesh vertex granularity.";
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

        const std::vector<VertexHandle> vertices = TopologyTraversal::vertices(mesh);
        const glm::mat4 world = node_world_matrix(scene, activeMesh);
        batch.markers.reserve(vertices.size());

        if (result != nullptr) {
            result->visitedVertexCount = vertices.size();
        }

        count_invalid_selected_vertices(mesh, selection, activeMesh, result);

        for (const VertexHandle handle : vertices) {
            if (should_skip_vertex(mesh, handle, options.skipHiddenComponents)) {
                continue;
            }

            const Vertex& vertex = mesh.vertex(handle);
            const VertexVisualState state = vertex_visual_state(
                handle,
                selection,
                activeMesh);

            batch.markers.push_back(
                make_marker(
                    transform_point(world, vertex.position),
                    state,
                    options.style));

            count_state(state, result);
        }

        if (result != nullptr) {
            result->message = result->has_geometry()
                ? "Topology overlay vertex markers built successfully."
                : "Active mesh produced no drawable topology overlay vertex markers.";
        }

        return batch;
    }

    TopologySurfaceOverlayBatches TopologyOverlayAdapter::build_active_mesh_face_surfaces(
        const EditorScene& scene,
        const SelectionState& selection,
        const TopologyOverlayOptions& options,
        TopologyOverlayResult* result)
    {
        if (result != nullptr) {
            *result = {};
            result->nodeId = selection.mesh().active_mesh();
        }

        TopologySurfaceOverlayBatches batches{};

        const SceneNodeId activeMesh = selection.mesh().active_mesh();
        if (!activeMesh.is_valid()) {
            if (result != nullptr) {
                result->message = "No active mesh available for face surface overlay.";
            }
            return batches;
        }

        if (!can_apply_face_selection(selection, activeMesh)) {
            if (result != nullptr) {
                result->message = "Face surface overlay is disabled outside active-mesh face granularity.";
            }
            return batches;
        }

        const MeshNode* meshNode = scene.find_mesh(activeMesh);
        if (meshNode == nullptr) {
            if (result != nullptr) {
                result->message = "Active mesh id did not resolve to a mesh node.";
            }
            return batches;
        }

        if (!meshNode->is_visible()) {
            if (result != nullptr) {
                result->message = "Active mesh is hidden.";
            }
            return batches;
        }

        const LEM& mesh = meshNode->mesh();
        if (mesh.empty()) {
            if (result != nullptr) {
                result->message = "Active mesh is empty.";
            }
            return batches;
        }

        const std::vector<FaceHandle> faces = TopologyTraversal::faces(mesh);
        const glm::mat4 model = node_world_matrix(scene, activeMesh);
        batches.hovered.modelMatrix = model;
        batches.selected.modelMatrix = model;

        if (result != nullptr) {
            result->visitedFaceCount = faces.size();
        }

        count_invalid_selected_faces(mesh, selection, activeMesh, result);

        for (const FaceHandle handle : faces) {
            if (should_skip_face(mesh, handle, options.skipHiddenComponents)) {
                continue;
            }

            const FaceVisualState state = face_visual_state(
                handle,
                selection,
                activeMesh);

            switch (state) {
            case FaceVisualState::Selected:
                append_face_surface(
                    batches.selected,
                    mesh,
                    handle,
                    options.style.selectedFaceColor,
                    result);
                break;
            case FaceVisualState::Hovered:
                append_face_surface(
                    batches.hovered,
                    mesh,
                    handle,
                    options.style.hoveredFaceColor,
                    result);
                break;
            case FaceVisualState::Normal:
                break;
            }

            count_state(state, result);
        }

        if (result != nullptr) {
            result->message = result->has_geometry()
                ? "Topology overlay face surfaces built successfully."
                : "Active mesh produced no drawable topology overlay face surfaces.";
        }

        return batches;
    }

} // namespace locus::editor
