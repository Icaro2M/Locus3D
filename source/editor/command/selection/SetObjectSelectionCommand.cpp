/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/selection/SetObjectSelectionCommand.h"

#include "editor/command/CommandContext.h"
#include "editor/scene/EditorScene.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/selection/SelectionState.h"

#include <algorithm>
#include <utility>

namespace locus::editor {

    namespace {

        [[nodiscard]] bool selectable(
            const EditorScene& scene,
            const SceneNodeId nodeId)
        {
            const SceneNode* node = scene.find_node(nodeId);
            return node != nullptr && node->is_selectable();
        }

        void append_unique(
            std::vector<SceneNodeId>& objects,
            const SceneNodeId nodeId)
        {
            if (nodeId.is_invalid()) {
                return;
            }

            if (std::find(objects.begin(), objects.end(), nodeId)
                == objects.end()) {
                objects.push_back(nodeId);
            }
        }

    } // namespace

    SetObjectSelectionCommand::SetObjectSelectionCommand(
        std::vector<SceneNodeId> objects,
        const SelectionOperation operation)
        : objects_(std::move(objects)),
        operation_(operation)
    {
    }

    std::string_view SetObjectSelectionCommand::name() const
    {
        return "Set Object Selection";
    }

    CommandResult SetObjectSelectionCommand::execute(
        CommandContext& context)
    {
        previousSelection_.capture(context.selection());

        std::vector<SceneNodeId> filtered;
        filtered.reserve(objects_.size());
        for (const SceneNodeId nodeId : objects_) {
            if (selectable(context.scene(), nodeId)) {
                append_unique(filtered, nodeId);
            }
        }

        ObjectSelection& selection = context.selection().objects();

        switch (operation_) {
        case SelectionOperation::Replace:
            selection.set(filtered);
            break;

        case SelectionOperation::Add: {
            std::vector<SceneNodeId> merged = selection.selected();
            for (const SceneNodeId nodeId : filtered) {
                append_unique(merged, nodeId);
            }
            selection.set(merged);
            break;
        }

        case SelectionOperation::Subtract:
            for (const SceneNodeId nodeId : filtered) {
                (void)selection.remove(nodeId);
            }
            break;

        case SelectionOperation::Toggle:
            for (const SceneNodeId nodeId : filtered) {
                (void)selection.toggle(nodeId);
            }
            break;
        }

        context.selection().mesh().clear();
        context.selection().set_scope(SelectionScope::Scene);
        context.selection().set_granularity(SelectionGranularity::Object);
        context.selection().mark_dirty();

        executed_ = true;

        return CommandResult::ok(
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Object selection set.");
    }

    CommandResult SetObjectSelectionCommand::undo(
        CommandContext& context)
    {
        if (!executed_ || !previousSelection_.is_valid()) {
            return CommandResult::fail(
                "Cannot undo object selection without a previous selection snapshot.");
        }

        previousSelection_.restore(context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Object selection restored.");
    }

} // namespace locus::editor
