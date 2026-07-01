/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"

#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that changes the active selection scope.
     */
    class SetSelectionScopeCommand final : public ICommand {
    public:
        /**
         * @brief Creates a command.
         *
         * @param scope New selection scope.
         */
        explicit SetSelectionScopeCommand(SelectionScope scope);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Executes the command.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Restores the previous selection scope and granularity.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

    private:
        SelectionScope scope_ = SelectionScope::Scene;
        SelectionScope previousScope_ = SelectionScope::Scene;
        SelectionGranularity previousGranularity_ = SelectionGranularity::Object;
        bool executed_ = false;
    };

} // namespace locus::editor