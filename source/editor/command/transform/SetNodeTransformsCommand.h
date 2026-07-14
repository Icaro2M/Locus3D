/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/transform/NodeTransformChange.h"

#include <string_view>
#include <vector>

namespace locus::editor {

    /**
     * @brief Command that applies absolute transforms to multiple scene nodes.
     *
     * This command is intended for commits produced by interactive transform
     * sessions. All affected nodes are stored in one command so one gizmo drag
     * corresponds to one undo history entry.
     */
    class SetNodeTransformsCommand final : public ICommand {
    public:
        /**
         * @brief Creates a batch transform command.
         *
         * @param changes Absolute before-and-after transform records.
         */
        explicit SetNodeTransformsCommand(
            std::vector<NodeTransformChange> changes);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]]
        std::string_view name() const override;

        /**
         * @brief Applies every final transform.
         *
         * @param context Command execution context.
         * @return Command result.
         */
        CommandResult execute(
            CommandContext& context) override;

        /**
         * @brief Restores every initial transform.
         *
         * @param context Command execution context.
         * @return Command result.
         */
        CommandResult undo(
            CommandContext& context) override;

        /**
         * @brief Reapplies every final transform.
         *
         * @param context Command execution context.
         * @return Command result.
         */
        CommandResult redo(
            CommandContext& context) override;

        /**
         * @brief Returns the stored transform changes.
         *
         * @return Read-only transform change list.
         */
        [[nodiscard]]
        const std::vector<NodeTransformChange>& changes() const;

    private:
        /**
         * @brief Selects which side of each transform change should be applied.
         */
        enum class ApplySide {
            Previous,
            Next
        };

        /**
         * @brief Validates all stored targets before modifying the scene.
         *
         * Validation happens before mutation so a missing target cannot leave the
         * scene partially transformed.
         *
         * @param context Command execution context.
         * @return Successful result when every target exists.
         */
        [[nodiscard]]
        CommandResult validate_targets(
            CommandContext& context) const;

        /**
         * @brief Applies one side of every transform record.
         *
         * @param context Command execution context.
         * @param side Snapshot side to apply.
         * @param message Success diagnostic message.
         * @return Command result.
         */
        CommandResult apply(
            CommandContext& context,
            ApplySide side,
            std::string_view message);

        std::vector<NodeTransformChange> changes_{};
    };

} // namespace locus::editor