/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/core/ToolContext.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/ICommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/sync/PickingSync.h"

#include <utility>

namespace locus::editor {

    CommandResult ToolContext::execute_command(
        std::unique_ptr<ICommand> command) {

        if (!has_command_services()) {
            return CommandResult::fail(
                "Tool command services are not available.");
        }

        if (!command) {
            return CommandResult::fail(
                "Cannot execute an empty tool command.");
        }

        return history_->execute(
            *dispatcher_,
            std::move(command));
    }

    SceneNodeId ToolContext::resolve_scene_node(
        graphics::PickingId pickingId) const {

        if (!has_picking_sync() ||
            !pickingId.is_valid()) {

            return SceneNodeId{};
        }

        return pickingSync_->scene_node_id(pickingId);
    }

} // namespace locus::editor