/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/scene/NodeMetadata.h"
#include "editor/scene/NodePivot.h"
#include "editor/scene/NodeTransform.h"
#include "editor/scene/NodeType.h"
#include "editor/scene/SceneNodeId.h"
#include "kernel/geometry/mesh/LEM.h"

#include <optional>
#include <string_view>
#include <vector>

namespace locus::editor {

    /**
     * @brief Command that duplicates a scene node and its descendants.
     *
     * The first execution allocates fresh node ids through the scene API. Undo
     * removes the duplicated subtree, and redo restores the same duplicated ids
     * from an internal snapshot.
     */
    class DuplicateNodeCommand final : public ICommand {
    public:
        /**
         * @brief Creates a duplicate command.
         *
         * @param node Root node to duplicate.
         */
        explicit DuplicateNodeCommand(SceneNodeId node);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Returns the duplicated root node id.
         *
         * @return Duplicated root id, or invalid before a successful execution.
         */
        [[nodiscard]] SceneNodeId duplicated_node() const;

        /**
         * @brief Duplicates the node subtree.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Removes the duplicated subtree.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

        /**
         * @brief Restores the duplicated subtree.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult redo(CommandContext& context) override;

    private:
        struct NodeSnapshot {
            SceneNodeId id{};
            SceneNodeId parent{};
            NodeType type = NodeType::Empty;
            NodeTransform transform{};
            NodePivot pivot{};
            NodeMetadata metadata{};
            std::optional<kernel::geometry::LEM> mesh{};
        };

        [[nodiscard]] SceneNodeId duplicate_node_recursive(
            CommandContext& context,
            SceneNodeId source,
            SceneNodeId duplicatedParent,
            bool isRoot);

        void capture_duplicated_node(CommandContext& context, SceneNodeId duplicated);
        [[nodiscard]] bool restore_duplicate_subtree(CommandContext& context);
        void mark_duplicate_dirty(CommandContext& context);

        SceneNodeId sourceNode_{};
        SceneNodeId duplicatedNode_{};
        std::vector<NodeSnapshot> snapshots_{};
        bool hasExecuted_ = false;
    };

}