/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/scene/SceneNodeId.h"

#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that changes the parent of a scene node.
     *
     * The command supports moving a node under another node or making it a root
     * node by passing an invalid parent id.
     */
    class ReparentNodeCommand final : public ICommand {
    public:
        /**
         * @brief Creates a command.
         *
         * @param child Node to move.
         * @param newParent New parent node, or invalid to make the child a root.
         */
        ReparentNodeCommand(SceneNodeId child, SceneNodeId newParent = {});

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Executes the reparent operation.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Restores the previous parent.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

        /**
         * @brief Reapplies the new parent.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult redo(CommandContext& context) override;

    private:
        CommandResult apply_parent(
            CommandContext& context,
            SceneNodeId parent,
            std::string_view message);

        SceneNodeId child_{};
        SceneNodeId newParent_{};
        SceneNodeId previousParent_{};
        bool captured_ = false;
    };

}