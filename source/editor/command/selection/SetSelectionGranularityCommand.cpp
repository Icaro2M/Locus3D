/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/selection/SetSelectionGranularityCommand.h"

#include "editor/selection/SelectionController.h"

namespace locus::editor {

    namespace {

        [[nodiscard]] const char* granularity_name(SelectionGranularity granularity)
        {
            switch (granularity) {
            case SelectionGranularity::Object:
                return "Object";
            case SelectionGranularity::Vertex:
                return "Vertex";
            case SelectionGranularity::Edge:
                return "Edge";
            case SelectionGranularity::Loop:
                return "Loop";
            case SelectionGranularity::Face:
                return "Face";
            }

            return "Unknown";
        }

    } // namespace

    SetSelectionGranularityCommand::SetSelectionGranularityCommand(SelectionGranularity granularity)
        : granularity_(granularity)
    {
    }

    std::string_view SetSelectionGranularityCommand::name() const
    {
        return "Set Selection Granularity";
    }

    CommandResult SetSelectionGranularityCommand::execute(CommandContext& context)
    {
        previousGranularity_ = context.selection().granularity();
        previousScope_ = context.selection().scope();

        if (previousGranularity_ == granularity_) {
            executed_ = true;
            return CommandResult::ok(
                EditorDirtyFlags::None,
                "Selection granularity unchanged.");
        }

        context.selection_controller().set_granularity(granularity_);
        executed_ = true;

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            std::string("Selection granularity changed to ") + granularity_name(granularity_) + ".");
    }

    CommandResult SetSelectionGranularityCommand::undo(CommandContext& context)
    {
        if (!executed_) {
            return CommandResult::fail("Cannot undo selection granularity change before execution.");
        }

        context.selection().set_granularity(previousGranularity_);
        context.selection().set_scope(previousScope_);

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            "Selection granularity restored.");
    }

} // namespace locus::editor