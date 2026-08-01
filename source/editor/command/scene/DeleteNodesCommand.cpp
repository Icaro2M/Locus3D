/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/DeleteNodesCommand.h"

#include "editor/command/CommandContext.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <algorithm>
#include <utility>

namespace locus::editor {

    namespace {

        [[nodiscard]] EditorDirtyFlags delete_dirty_flags()
        {
            return EditorDirtyFlags::Scene
                | EditorDirtyFlags::Selection
                | EditorDirtyFlags::Mesh
                | EditorDirtyFlags::Render
                | EditorDirtyFlags::Picking;
        }

        [[nodiscard]] bool contains_node(
            const std::vector<SceneNodeId>& nodes,
            SceneNodeId node)
        {
            return std::find(nodes.begin(), nodes.end(), node)
                != nodes.end();
        }

    } // namespace

    DeleteNodesCommand::DeleteNodesCommand(
        std::vector<SceneNodeId> nodes)
        : nodes_(std::move(nodes))
    {
    }

    std::string_view DeleteNodesCommand::name() const
    {
        return "Delete Nodes";
    }

    CommandResult DeleteNodesCommand::execute(CommandContext& context)
    {
        if (executed_) {
            return redo(context);
        }

        if (!prepare_commands(context)) {
            return CommandResult::fail(
                "Cannot delete an empty or invalid node selection.");
        }

        std::size_t executedCount = 0u;

        for (DeleteNodeCommand& command : commands_) {
            const CommandResult result = command.execute(context);

            if (!result.success) {
                while (executedCount > 0u) {
                    --executedCount;
                    (void)commands_[executedCount].undo(context);
                }

                return CommandResult::fail(
                    result.message.empty()
                    ? "Failed to delete selected nodes."
                    : result.message);
            }

            ++executedCount;
        }

        executed_ = true;

        return CommandResult::ok(
            delete_dirty_flags(),
            "Nodes deleted.");
    }

    CommandResult DeleteNodesCommand::undo(CommandContext& context)
    {
        if (!executed_) {
            return CommandResult::fail(
                "Cannot undo node deletion before execution.");
        }

        for (auto iterator = commands_.rbegin();
            iterator != commands_.rend();
            ++iterator) {
            const CommandResult result = iterator->undo(context);

            if (!result.success) {
                return CommandResult::fail(
                    result.message.empty()
                    ? "Failed to restore deleted nodes."
                    : result.message);
            }
        }

        return CommandResult::ok(
            delete_dirty_flags(),
            "Node deletion undone.");
    }

    CommandResult DeleteNodesCommand::redo(CommandContext& context)
    {
        if (!executed_) {
            return CommandResult::fail(
                "Cannot redo node deletion before execution.");
        }

        for (DeleteNodeCommand& command : commands_) {
            const CommandResult result = command.redo(context);

            if (!result.success) {
                return CommandResult::fail(
                    result.message.empty()
                    ? "Failed to delete selected nodes."
                    : result.message);
            }
        }

        return CommandResult::ok(
            delete_dirty_flags(),
            "Nodes deleted.");
    }

    bool DeleteNodesCommand::prepare_commands(CommandContext& context)
    {
        if (prepared_) {
            return !commands_.empty();
        }

        std::vector<SceneNodeId> uniqueNodes{};
        uniqueNodes.reserve(nodes_.size());

        for (const SceneNodeId node : nodes_) {
            if (node.is_invalid()
                || contains_node(uniqueNodes, node)
                || !context.scene().find_node(node)) {
                continue;
            }

            if (is_shadowed_by_selected_ancestor(context, node)) {
                continue;
            }

            uniqueNodes.push_back(node);
        }

        commands_.clear();
        commands_.reserve(uniqueNodes.size());

        for (const SceneNodeId node : uniqueNodes) {
            commands_.emplace_back(node);
        }

        prepared_ = true;
        return !commands_.empty();
    }

    bool DeleteNodesCommand::is_shadowed_by_selected_ancestor(
        CommandContext& context,
        SceneNodeId node) const
    {
        for (const SceneNodeId candidate : nodes_) {
            if (candidate != node
                && context.scene().tree().is_ancestor(candidate, node)) {
                return true;
            }
        }

        return false;
    }

} // namespace locus::editor
