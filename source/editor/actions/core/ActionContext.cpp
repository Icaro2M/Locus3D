/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/core/ActionContext.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/ICommand.h"
#include "editor/history/HistoryStack.h"

#include <utility>

namespace locus::editor {

    CommandResult ActionContext::execute_command(
        std::unique_ptr<ICommand> command) {
        if (!command) {
            return CommandResult::fail(
                "Cannot execute an empty action command.");
        }

        return history_->execute(
            *dispatcher_,
            std::move(command));
    }

} // namespace locus::editor