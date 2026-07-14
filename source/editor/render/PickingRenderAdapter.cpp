/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/PickingRenderAdapter.h"

#include "editor/scene/SceneNodeId.h"
#include "editor/sync/PickingSync.h"
#include "graphics/picking/PickingId.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

namespace locus::editor {

    bool PickingRenderAdapter::apply_to_object(
        graphics::RenderObject& object,
        const PickingSync& sync
    ) {
        object.pickingId = graphics::PickingId::invalid();

        if (object.id == 0) {
            return false;
        }

        const SceneNodeId nodeId{
            static_cast<graphics::u64>(object.id)
        };

        if (!nodeId.is_valid()) {
            return false;
        }

        const graphics::PickingId pickingId =
            sync.picking_id(nodeId);

        if (!pickingId.is_valid()) {
            return false;
        }

        object.pickingId = pickingId;
        return true;
    }

    void PickingRenderAdapter::apply_to_scene(
        graphics::RenderScene& scene,
        const PickingSync& sync,
        PickingRenderResult* result
    ) {
        if (result) {
            *result = {};
        }

        for (graphics::RenderObject& object : scene.objects()) {
            if (result) {
                ++result->visitedObjectCount;
            }

            if (object.id == 0) {
                object.pickingId =
                    graphics::PickingId::invalid();

                if (result) {
                    ++result->invalidObjectCount;
                }

                continue;
            }

            if (apply_to_object(object, sync)) {
                if (result) {
                    ++result->assignedObjectCount;
                }

                continue;
            }

            if (result) {
                ++result->unmappedObjectCount;
            }
        }

        if (!result) {
            return;
        }

        if (scene.empty()) {
            result->message =
                "Picking mappings applied to an empty render scene.";
        }
        else if (result->assignedObjectCount == 0) {
            result->message =
                "No render object matched an active picking mapping.";
        }
        else {
            result->message =
                "Picking mappings applied to render objects successfully.";
        }
    }

} // namespace locus::editor