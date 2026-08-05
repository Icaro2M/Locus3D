/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/core/ToolContext.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/ICommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/sync/PickingSync.h"
#include "editor/tools/core/ToolEvent.h"
#include "kernel/geometry/queries/BoundsQuery.h"
#include "kernel/geometry/queries/SelectionQuery.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/math/Ray.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <utility>

namespace locus::editor {

    namespace {

        [[nodiscard]] glm::mat4 node_world_matrix(
            const EditorScene& scene,
            const SceneNodeId nodeId)
        {
            const SceneNode* node = scene.find_node(nodeId);
            if (node == nullptr) {
                return glm::mat4{ 1.0f };
            }

            const glm::mat4 local = node->transform().matrix();
            if (!node->parent().is_valid()) {
                return local;
            }

            return node_world_matrix(scene, node->parent()) * local;
        }

        template <typename T>
        void append_unique(std::vector<T>& items, T item)
        {
            if (std::find(items.begin(), items.end(), item) == items.end()) {
                items.push_back(item);
            }
        }

        [[nodiscard]] bool project_point(
            const glm::mat4& viewProjection,
            const glm::vec2& viewportSize,
            const glm::vec3& worldPoint,
            glm::vec2& screenPoint)
        {
            if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f) {
                return false;
            }

            const glm::vec4 clip =
                viewProjection * glm::vec4(worldPoint, 1.0f);
            if (clip.w <= 0.0f) {
                return false;
            }

            const glm::vec3 ndc = glm::vec3(clip) / clip.w;
            screenPoint.x = (ndc.x * 0.5f + 0.5f) * viewportSize.x;
            screenPoint.y = (ndc.y * 0.5f + 0.5f) * viewportSize.y;
            return true;
        }

        [[nodiscard]] std::vector<glm::vec2> project_bounds(
            const kernel::math::Bounds& bounds,
            const glm::mat4& viewProjection,
            const glm::vec2& viewportSize)
        {
            std::vector<glm::vec2> projected;
            if (!bounds.is_valid()) {
                return projected;
            }

            const glm::vec3 min = bounds.min;
            const glm::vec3 max = bounds.max;
            const glm::vec3 corners[] = {
                { min.x, min.y, min.z },
                { max.x, min.y, min.z },
                { min.x, max.y, min.z },
                { max.x, max.y, min.z },
                { min.x, min.y, max.z },
                { max.x, min.y, max.z },
                { min.x, max.y, max.z },
                { max.x, max.y, max.z }
            };

            for (const glm::vec3& corner : corners) {
                glm::vec2 screen{};
                if (project_point(
                        viewProjection,
                        viewportSize,
                        corner,
                        screen)) {
                    projected.push_back(screen);
                }
            }

            return projected;
        }

        [[nodiscard]] bool points_match_rect(
            const std::vector<glm::vec2>& points,
            const ScreenSelectionRect& rect,
            const SelectionContainment containment)
        {
            if (points.empty()) {
                return false;
            }

            if (containment == SelectionContainment::FullyContained) {
                return std::all_of(
                    points.begin(),
                    points.end(),
                    [&](const glm::vec2& point) {
                        return rect.contains(point);
                    });
            }

            return std::any_of(
                points.begin(),
                points.end(),
                [&](const glm::vec2& point) {
                    return rect.contains(point);
                });
        }

        [[nodiscard]] kernel::geometry::SelectionElementMask mask_for_granularity(
            const SelectionGranularity granularity)
        {
            using kernel::geometry::SelectionElementMask;

            switch (granularity) {
            case SelectionGranularity::Vertex:
                return SelectionElementMask::Vertex;
            case SelectionGranularity::Edge:
                return SelectionElementMask::Edge;
            case SelectionGranularity::Face:
                return SelectionElementMask::Face;
            case SelectionGranularity::Loop:
            case SelectionGranularity::Object:
                return SelectionElementMask::None;
            }

            return SelectionElementMask::None;
        }

    } // namespace

    CommandResult ToolContext::execute_command(
        std::unique_ptr<ICommand> command) {

        if (!has_command_services()) {
            return CommandResult::fail(
                "Tool command services are not available.");
        }

        if (!command) {
            return CommandResult::fail(
                "Cannot execute an empty tool command.");
        }

        return history_->execute(
            *dispatcher_,
            std::move(command));
    }

    std::vector<SceneNodeId> ToolContext::resolve_scene_nodes(
        const std::vector<graphics::PickingId>& pickingIds) const
    {
        std::vector<SceneNodeId> result;

        for (const graphics::PickingId pickingId : pickingIds) {
            const SceneNodeId nodeId = resolve_scene_node(pickingId);
            if (nodeId.is_valid()) {
                append_unique(result, nodeId);
            }
        }

        return result;
    }

    SceneNodeId ToolContext::resolve_scene_node(
        graphics::PickingId pickingId) const {

        if (!has_picking_sync() ||
            !pickingId.is_valid()) {

            return SceneNodeId{};
        }

        return pickingSync_->scene_node_id(pickingId);
    }

    kernel::geometry::SelectionHit ToolContext::resolve_active_mesh_component(
        const ToolEvent& event) const
    {
        const SelectionGranularity granularity =
            selection().granularity();

        if (!is_mesh_granularity(granularity)) {
            return kernel::geometry::SelectionHit::miss();
        }

        const SceneNodeId activeMesh =
            selection().mesh().active_mesh();

        return resolve_mesh_component(activeMesh, event);
    }

    kernel::geometry::SelectionHit ToolContext::resolve_mesh_component(
        const SceneNodeId meshNodeId,
        const ToolEvent& event) const
    {
        if (!event.is_pointer_event()) {
            return kernel::geometry::SelectionHit::miss();
        }

        const SelectionGranularity granularity =
            selection().granularity();

        if (!is_mesh_granularity(granularity) ||
            !meshNodeId.is_valid()) {
            return kernel::geometry::SelectionHit::miss();
        }

        const MeshNode* meshNode =
            scene().find_mesh(meshNodeId);

        if (meshNode == nullptr || !meshNode->is_selectable()) {
            return kernel::geometry::SelectionHit::miss();
        }

        const kernel::geometry::SelectionElementMask mask =
            mask_for_granularity(granularity);

        if (mask == kernel::geometry::SelectionElementMask::None) {
            return kernel::geometry::SelectionHit::miss();
        }

        const glm::mat4 inverseWorld =
            glm::inverse(node_world_matrix(scene(), meshNodeId));

        const glm::vec4 localOrigin =
            inverseWorld * glm::vec4(event.pointer.worldRay.origin, 1.0f);
        const glm::vec4 localDirection =
            inverseWorld * glm::vec4(event.pointer.worldRay.direction, 0.0f);

        kernel::math::Ray localRay{};
        localRay.origin = glm::vec3(localOrigin);
        localRay.direction = glm::vec3(localDirection);

        kernel::geometry::SelectionQueryOptions options{};
        options.mask = mask;
        options.vertexRadius = 0.04f * event.pointer.visualScale;
        options.edgeRadius = 0.025f * event.pointer.visualScale;
        options.preferVertices = granularity == SelectionGranularity::Vertex;
        options.preferEdges = granularity == SelectionGranularity::Edge;

        return kernel::geometry::SelectionQuery::pick_by_ray(
            meshNode->mesh(),
            localRay,
            options);
    }

    std::vector<SceneNodeId> ToolContext::resolve_objects_in_rect(
        const ScreenSelectionRect& rect,
        const ToolEvent& event,
        const SelectionContainment containment) const
    {
        std::vector<SceneNodeId> result =
            resolve_scene_nodes(event.pointer.regionalPickingIds);

        if (!result.empty()) {
            return result;
        }

        const std::vector<SceneNodeId> nodeIds = scene().tree().node_ids();
        for (const SceneNodeId nodeId : nodeIds) {
            const MeshNode* meshNode = scene().find_mesh(nodeId);
            if (meshNode == nullptr || !meshNode->is_selectable()) {
                continue;
            }

            const kernel::math::Bounds localBounds =
                kernel::geometry::BoundsQuery::mesh_bounds(meshNode->mesh());
            const kernel::math::Bounds worldBounds =
                localBounds.transformed(node_world_matrix(scene(), nodeId));

            const std::vector<glm::vec2> projected =
                project_bounds(
                    worldBounds,
                    event.pointer.viewProjection,
                    event.pointer.viewportSize);

            if (points_match_rect(projected, rect, containment)) {
                append_unique(result, nodeId);
            }
        }

        return result;
    }

    std::vector<kernel::geometry::SelectionHit>
    ToolContext::resolve_active_mesh_components(
        const ScreenSelectionRect& rect,
        const ToolEvent& event,
        const SelectionContainment containment) const
    {
        std::vector<kernel::geometry::SelectionHit> result;
        const SelectionGranularity granularity = selection().granularity();
        const SceneNodeId activeMesh = selection().mesh().active_mesh();
        const MeshNode* meshNode = scene().find_mesh(activeMesh);

        if (!is_mesh_granularity(granularity) ||
            meshNode == nullptr ||
            !meshNode->is_selectable()) {
            return result;
        }

        const glm::mat4 world = node_world_matrix(scene(), activeMesh);
        const kernel::geometry::LEM& mesh = meshNode->mesh();

        if (granularity == SelectionGranularity::Vertex) {
            for (const kernel::geometry::VertexHandle vertexHandle :
                kernel::geometry::TopologyTraversal::vertices(mesh)) {
                const auto& vertex = mesh.vertex(vertexHandle);
                if (vertex.hidden) {
                    continue;
                }

                glm::vec2 screen{};
                if (project_point(
                        event.pointer.viewProjection,
                        event.pointer.viewportSize,
                        glm::vec3(world * glm::vec4(vertex.position, 1.0f)),
                        screen) &&
                    rect.contains(screen)) {
                    result.push_back(
                        kernel::geometry::SelectionHit::vertex_hit(
                            vertexHandle,
                            0.0f,
                            vertex.position));
                }
            }
            return result;
        }

        if (granularity == SelectionGranularity::Edge) {
            for (const kernel::geometry::EdgeHandle edgeHandle :
                kernel::geometry::TopologyTraversal::edges(mesh)) {
                const auto& edge = mesh.edge(edgeHandle);
                if (edge.hidden ||
                    !mesh.is_valid(edge.vertexA) ||
                    !mesh.is_valid(edge.vertexB)) {
                    continue;
                }

                const auto& a = mesh.vertex(edge.vertexA);
                const auto& b = mesh.vertex(edge.vertexB);
                glm::vec2 screenA{};
                glm::vec2 screenB{};
                if (!project_point(
                        event.pointer.viewProjection,
                        event.pointer.viewportSize,
                        glm::vec3(world * glm::vec4(a.position, 1.0f)),
                        screenA) ||
                    !project_point(
                        event.pointer.viewProjection,
                        event.pointer.viewportSize,
                        glm::vec3(world * glm::vec4(b.position, 1.0f)),
                        screenB)) {
                    continue;
                }

                const bool selected =
                    containment == SelectionContainment::FullyContained
                    ? rect.contains(screenA) && rect.contains(screenB)
                    : rect.contains(screenA) ||
                        rect.contains(screenB) ||
                        rect.intersects_segment(screenA, screenB);

                if (selected) {
                    result.push_back(
                        kernel::geometry::SelectionHit::edge_hit(
                            edgeHandle,
                            0.0f,
                            (a.position + b.position) * 0.5f));
                }
            }
            return result;
        }

        if (granularity == SelectionGranularity::Face) {
            for (const kernel::geometry::FaceHandle faceHandle :
                kernel::geometry::TopologyTraversal::faces(mesh)) {
                const auto& face = mesh.face(faceHandle);
                if (face.hidden) {
                    continue;
                }

                std::vector<glm::vec2> projected;
                for (const kernel::geometry::VertexHandle vertexHandle :
                    kernel::geometry::TopologyTraversal::face_vertices(
                        mesh,
                        faceHandle)) {
                    const auto& vertex = mesh.vertex(vertexHandle);
                    glm::vec2 screen{};
                    if (project_point(
                            event.pointer.viewProjection,
                            event.pointer.viewportSize,
                            glm::vec3(world * glm::vec4(vertex.position, 1.0f)),
                            screen)) {
                        projected.push_back(screen);
                    }
                }

                if (points_match_rect(projected, rect, containment)) {
                    glm::vec3 center{ 0.0f };
                    std::size_t count = 0u;
                    for (const kernel::geometry::VertexHandle vertexHandle :
                        kernel::geometry::TopologyTraversal::face_vertices(
                            mesh,
                            faceHandle)) {
                        center += mesh.vertex(vertexHandle).position;
                        ++count;
                    }
                    if (count > 0u) {
                        center /= static_cast<float>(count);
                    }

                    result.push_back(
                        kernel::geometry::SelectionHit::face_hit(
                            faceHandle,
                            0.0f,
                            center));
                }
            }
        }

        return result;
    }

} // namespace locus::editor
