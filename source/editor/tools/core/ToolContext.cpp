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
#include "kernel/geometry/queries/SelectionQuery.h"
#include "kernel/math/Ray.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

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
        if (!event.is_pointer_event()) {
            return kernel::geometry::SelectionHit::miss();
        }

        const SelectionGranularity granularity =
            selection().granularity();

        if (!is_mesh_granularity(granularity)) {
            return kernel::geometry::SelectionHit::miss();
        }

        const SceneNodeId activeMesh =
            selection().mesh().active_mesh();

        if (!activeMesh.is_valid()) {
            return kernel::geometry::SelectionHit::miss();
        }

        const MeshNode* meshNode =
            scene().find_mesh(activeMesh);

        if (meshNode == nullptr || !meshNode->is_selectable()) {
            return kernel::geometry::SelectionHit::miss();
        }

        const kernel::geometry::SelectionElementMask mask =
            mask_for_granularity(granularity);

        if (mask == kernel::geometry::SelectionElementMask::None) {
            return kernel::geometry::SelectionHit::miss();
        }

        const glm::mat4 inverseWorld =
            glm::inverse(node_world_matrix(scene(), activeMesh));

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

} // namespace locus::editor
