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
     * @brief Command that changes the active selection granularity.
     */
    class SetSelectionGranularityCommand final : public ICommand {
    public:
        /**
         * @brief Creates a command.
         *
         * @param granularity New selection granularity.
         */
        explicit SetSelectionGranularityCommand(SelectionGranularity granularity);

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
         * @brief Restores the previous selection granularity and scope.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

    private:
        SelectionGranularity granularity_ = SelectionGranularity::Object;
        SelectionGranularity previousGranularity_ = SelectionGranularity::Object;
        SelectionScope previousScope_ = SelectionScope::Scene;
        bool executed_ = false;
    };

} // namespace locus::editor