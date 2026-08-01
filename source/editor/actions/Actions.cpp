/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/Actions.h"

#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionId.h"
#include "editor/actions/edit/RegisterEditActions.h"
#include "editor/actions/mesh/edge/RegisterEdgeActions.h"
#include "editor/actions/mesh/face/RegisterFaceActions.h"
#include "editor/actions/mesh/topology/RegisterTopologyActions.h"
#include "editor/actions/mesh/vertex/RegisterVertexActions.h"
#include "editor/actions/scene/RegisterSceneActions.h"

#include <vector>

namespace locus::editor {

    namespace {

        [[nodiscard]] std::vector<ActionId> snapshot_action_ids(
            const ActionRegistry& registry) {
            return registry.action_ids();
        }

        void rollback_to_snapshot(
            ActionRegistry& registry,
            const std::vector<ActionId>& existingIds) {
            const std::vector<ActionId> currentIds =
                registry.action_ids();

            for (const ActionId& id : currentIds) {
                bool existed = false;

                for (const ActionId& existingId : existingIds) {
                    if (id == existingId) {
                        existed = true;
                        break;
                    }
                }

                if (!existed) {
                    registry.unregister_action(id);
                }
            }
        }

    } // namespace

    bool register_default_actions(ActionRegistry& registry) {
        const std::vector<ActionId> existingIds =
            snapshot_action_ids(registry);

        if (!register_vertex_actions(registry)
            || !register_edge_actions(registry)
            || !register_face_actions(registry)
            || !register_topology_actions(registry)
            || !register_scene_actions(registry)
            || !register_edit_actions(registry)) {
            rollback_to_snapshot(registry, existingIds);
            return false;
        }

        return true;
    }

} // namespace locus::editor
