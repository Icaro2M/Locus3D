/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/selection/SetSelectionScopeCommand.h"

namespace locus::editor {

    namespace {

        [[nodiscard]] const char* scope_name(SelectionScope scope)
        {
            switch (scope) {
            case SelectionScope::Scene:
                return "Scene";
            case SelectionScope::ActiveMesh:
                return "ActiveMesh";
            }

            return "Unknown";
        }

    } // namespace

    SetSelectionScopeCommand::SetSelectionScopeCommand(SelectionScope scope)
        : scope_(scope)
    {
    }

    std::string_view SetSelectionScopeCommand::name() const
    {
        return "Set Selection Scope";
    }

    CommandResult SetSelectionScopeCommand::execute(CommandContext& context)
    {
        previousScope_ = context.selection().scope();
        previousGranularity_ = context.selection().granularity();

        if (previousScope_ == scope_) {
            executed_ = true;
            return CommandResult::ok(
                EditorDirtyFlags::None,
                "Selection scope unchanged.");
        }

        context.selection().set_scope(scope_);
        executed_ = true;

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            std::string("Selection scope changed to ") + scope_name(scope_) + ".");
    }

    CommandResult SetSelectionScopeCommand::undo(CommandContext& context)
    {
        if (!executed_) {
            return CommandResult::fail("Cannot undo selection scope change before execution.");
        }

        context.selection().set_granularity(previousGranularity_);
        context.selection().set_scope(previousScope_);

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            "Selection scope restored.");
    }

} // namespace locus::editor