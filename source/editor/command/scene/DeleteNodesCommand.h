/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/scene/DeleteNodeCommand.h"
#include "editor/scene/SceneNodeId.h"

#include <string_view>
#include <vector>

namespace locus::editor {

    /**
     * @brief Deletes multiple scene node subtrees as one undoable command.
     *
     * The command delegates per-subtree capture, deletion, restoration, and
     * selection cleanup to DeleteNodeCommand while preserving a single history
     * entry for multi-object Delete.
     */
    class DeleteNodesCommand final : public ICommand {
    public:
        explicit DeleteNodesCommand(std::vector<SceneNodeId> nodes);

        [[nodiscard]] std::string_view name() const override;

        CommandResult execute(CommandContext& context) override;

        CommandResult undo(CommandContext& context) override;

        CommandResult redo(CommandContext& context) override;

    private:
        [[nodiscard]] bool prepare_commands(CommandContext& context);
        [[nodiscard]] bool is_shadowed_by_selected_ancestor(
            CommandContext& context,
            SceneNodeId node) const;

        std::vector<SceneNodeId> nodes_{};
        std::vector<DeleteNodeCommand> commands_{};
        bool prepared_ = false;
        bool executed_ = false;
    };

} // namespace locus::editor
