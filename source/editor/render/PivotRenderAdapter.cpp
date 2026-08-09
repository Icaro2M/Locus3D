/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/PivotRenderAdapter.h"

#include "editor/scene/EditorScene.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/selection/SelectionState.h"
#include "editor/tools/transform/PivotTool.h"
#include "editor/transform/TransformPivotResolver.h"

namespace locus::editor {

    namespace {

        [[nodiscard]] SceneNodeId resolve_active_object(
            const EditorScene& scene,
            const SelectionState& selection)
        {
            if (selection.scope() != SelectionScope::Scene ||
                selection.granularity() != SelectionGranularity::Object) {
                return {};
            }

            const SceneNodeId active =
                selection.objects().active();
            if (active.is_valid() &&
                selection.objects().contains(active) &&
                scene.find_node(active) != nullptr) {
                return active;
            }

            if (selection.objects().size() == 1u) {
                const SceneNodeId selected =
                    selection.objects().selected().front();
                if (scene.find_node(selected) != nullptr) {
                    return selected;
                }
            }

            return {};
        }

    } // namespace

    graphics::PivotDrawData PivotRenderAdapter::build_active_object_pivot(
        const EditorScene& scene,
        const SelectionState& selection,
        const PivotTool* activePivotTool)
    {
        graphics::PivotDrawData data{};

        const SceneNodeId node =
            resolve_active_object(scene, selection);
        if (node.is_invalid()) {
            return data;
        }

        data.position =
            TransformPivotResolver::node_pivot_position(
                scene,
                node);
        data.visible = true;

        if (activePivotTool != nullptr &&
            activePivotTool->active_node() == node) {
            if (activePivotTool->dragging()) {
                data.state = graphics::PivotVisualState::Active;
            }
            else if (activePivotTool->hovered()) {
                data.state = graphics::PivotVisualState::Hovered;
            }
        }

        return data;
    }

} // namespace locus::editor
