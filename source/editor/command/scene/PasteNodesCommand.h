/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/io/SceneFragment.h"
#include "editor/selection/SelectionSerializer.h"

#include <string_view>
#include <unordered_map>
#include <vector>

namespace locus::editor {

    class PasteNodesCommand final : public ICommand {
    public:
        explicit PasteNodesCommand(SceneFragment fragment);

        [[nodiscard]] std::string_view name() const override;
        [[nodiscard]] const std::vector<SceneNodeId>& pasted_nodes() const;

        CommandResult execute(CommandContext& context) override;
        CommandResult undo(CommandContext& context) override;
        CommandResult redo(CommandContext& context) override;

    private:
        struct NodeSnapshot {
            SceneNodeId id{};
            SceneNodeId parent{};
            SerializedNode node{};
        };

        [[nodiscard]] CommandResult execute_first(CommandContext& context);
        [[nodiscard]] bool restore_from_snapshots(CommandContext& context);
        [[nodiscard]] bool reparent_created_nodes(CommandContext& context);
        void remove_created_nodes(CommandContext& context);
        void select_pasted_nodes(CommandContext& context) const;
        void mark_pasted_dirty(CommandContext& context) const;

        SceneFragment fragment_{};
        std::unordered_map<SceneFragmentNodeId, SceneNodeId> remap_{};
        std::vector<NodeSnapshot> snapshots_{};
        std::vector<SceneNodeId> pastedNodes_{};
        SelectionSnapshot previousSelection_{};
        bool executed_ = false;
    };

} // namespace locus::editor
