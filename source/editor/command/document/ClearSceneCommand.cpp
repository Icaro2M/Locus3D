/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/document/ClearSceneCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/EmptyNode.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"

#include <memory>
#include <utility>

namespace locus::editor {

    std::string_view ClearSceneCommand::name() const
    {
        return "Clear Scene";
    }

    CommandResult ClearSceneCommand::execute(CommandContext& context)
    {
        if (context.scene().tree().empty()) {
            return CommandResult::fail("Cannot clear an already empty scene.");
        }

        if (!captured_) {
            capture_scene(context);
        }

        if (snapshots_.empty()) {
            return CommandResult::fail("Failed to capture scene before clearing.");
        }

        context.selection().clear();
        context.scene().clear();

        return CommandResult::ok(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking |
            EditorDirtyFlags::Manufacturing,
            "Scene cleared.");
    }

    CommandResult ClearSceneCommand::undo(CommandContext& context)
    {
        if (!captured_ || snapshots_.empty()) {
            return CommandResult::fail("Cannot undo scene clear without a captured scene.");
        }

        if (!context.scene().tree().empty()) {
            return CommandResult::fail("Cannot restore cleared scene because the current scene is not empty.");
        }

        if (!restore_scene(context)) {
            return CommandResult::fail("Failed to restore cleared scene.");
        }

        objectSelection_.restore(context.selection());
        meshSelection_.restore(context.selection());
        mark_restored_scene_dirty(context);

        return CommandResult::ok(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking |
            EditorDirtyFlags::Manufacturing,
            "Scene clear undone.");
    }

    CommandResult ClearSceneCommand::redo(CommandContext& context)
    {
        if (!captured_) {
            return CommandResult::fail("Cannot redo scene clear before execution.");
        }

        context.selection().clear();
        context.scene().clear();

        return CommandResult::ok(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking |
            EditorDirtyFlags::Manufacturing,
            "Scene cleared.");
    }

    void ClearSceneCommand::capture_scene(CommandContext& context)
    {
        snapshots_.clear();
        objectSelection_.capture(context.selection());
        meshSelection_.capture(context.selection());

        for (SceneNodeId root : context.scene().tree().roots()) {
            capture_node_recursive(context, root);
        }

        captured_ = !snapshots_.empty();
    }

    void ClearSceneCommand::capture_node_recursive(CommandContext& context, SceneNodeId id)
    {
        const SceneNode* node = context.scene().find_node(id);
        if (!node) {
            return;
        }

        NodeSnapshot snapshot{};
        snapshot.id = node->id();
        snapshot.parent = node->parent();
        snapshot.type = node->type();
        snapshot.transform = node->transform();
        snapshot.pivot = node->pivot();
        snapshot.metadata = node->metadata();

        if (const auto* meshNode = dynamic_cast<const MeshNode*>(node)) {
            snapshot.mesh = meshNode->mesh();
        }

        snapshots_.push_back(std::move(snapshot));

        for (SceneNodeId child : node->children()) {
            capture_node_recursive(context, child);
        }
    }

    bool ClearSceneCommand::restore_scene(CommandContext& context) const
    {
        for (const NodeSnapshot& snapshot : snapshots_) {
            if (context.scene().find_node(snapshot.id)) {
                return false;
            }
        }

        for (const NodeSnapshot& snapshot : snapshots_) {
            std::unique_ptr<SceneNode> node;

            switch (snapshot.type) {
            case NodeType::Empty:
                node = std::make_unique<EmptyNode>(snapshot.id, snapshot.metadata.name);
                break;

            case NodeType::Mesh: {
                auto meshNode = std::make_unique<MeshNode>(snapshot.id, snapshot.metadata.name);
                if (snapshot.mesh.has_value()) {
                    meshNode->mesh() = snapshot.mesh.value();
                }
                node = std::move(meshNode);
                break;
            }

            default:
                return false;
            }

            node->transform() = snapshot.transform;
            node->pivot() = snapshot.pivot;
            node->metadata() = snapshot.metadata;

            if (context.scene().tree().insert_node(std::move(node)).is_invalid()) {
                return false;
            }
        }

        for (const NodeSnapshot& snapshot : snapshots_) {
            if (snapshot.parent.is_valid()) {
                if (!context.scene().reparent(snapshot.id, snapshot.parent)) {
                    return false;
                }
            }
        }

        return true;
    }

    void ClearSceneCommand::mark_restored_scene_dirty(CommandContext& context) const
    {
        for (const NodeSnapshot& snapshot : snapshots_) {
            if (SceneNode* node = context.scene().find_node(snapshot.id)) {
                node->mark_dirty(
                    EditorDirtyFlags::Scene |
                    EditorDirtyFlags::Mesh |
                    EditorDirtyFlags::Render |
                    EditorDirtyFlags::Picking |
                    EditorDirtyFlags::Manufacturing);
            }
        }
    }

} // namespace locus::editor