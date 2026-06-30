/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CommandTestSuite.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/ICommand.h"

namespace {

class CountingCommand final : public locus::editor::ICommand {
public:
    explicit CountingCommand(bool undoable = true)
        : undoable_(undoable)
    {
    }

    [[nodiscard]] std::string_view name() const override
    {
        return "Counting Command";
    }

    [[nodiscard]] bool is_undoable() const override
    {
        return undoable_;
    }

    locus::editor::CommandResult execute(locus::editor::CommandContext& context) override
    {
        ++executeCount;
        context.scene().create_empty("Executed");
        return locus::editor::CommandResult::ok(
            locus::editor::EditorDirtyFlags::Scene,
            "executed");
    }

    locus::editor::CommandResult undo(locus::editor::CommandContext&) override
    {
        ++undoCount;
        return locus::editor::CommandResult::ok(
            locus::editor::EditorDirtyFlags::Selection,
            "undone");
    }

    locus::editor::CommandResult redo(locus::editor::CommandContext&) override
    {
        ++redoCount;
        return locus::editor::CommandResult::ok(
            locus::editor::EditorDirtyFlags::Mesh,
            "redone");
    }

    int executeCount = 0;
    int undoCount = 0;
    int redoCount = 0;

private:
    bool undoable_ = true;
};

} // namespace

namespace locus::tests {

TestResult run_command_dispatcher_tests()
{
    editor::Editor editor;
    editor.clear_dirty();

    editor::CommandDispatcher dispatcher(editor);
    CountingCommand command;

    const editor::CommandResult executeResult = dispatcher.execute(command);
    if (!executeResult ||
        executeResult.message != "executed" ||
        dispatcher.last_result().message != "executed" ||
        command.executeCount != 1 ||
        !editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Scene)) {
        return TestResult::fail("execute should run command, store result, and apply dirty flags");
    }

    editor.clear_dirty();
    const editor::CommandResult undoResult = dispatcher.undo(command);
    if (!undoResult ||
        undoResult.message != "undone" ||
        command.undoCount != 1 ||
        !editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Selection)) {
        return TestResult::fail("undo should run undoable commands and apply dirty flags");
    }

    editor.clear_dirty();
    const editor::CommandResult redoResult = dispatcher.redo(command);
    if (!redoResult ||
        redoResult.message != "redone" ||
        command.redoCount != 1 ||
        !editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Mesh)) {
        return TestResult::fail("redo should run command redo and apply dirty flags");
    }

    editor.clear_dirty();
    CountingCommand notUndoable(false);
    const editor::CommandResult undoBlocked = dispatcher.undo(notUndoable);
    if (undoBlocked ||
        undoBlocked.message.empty() ||
        notUndoable.undoCount != 0 ||
        editor.dirty_flags() != editor::EditorDirtyFlags::None) {
        return TestResult::fail("undo should reject non-undoable commands without dirtying editor");
    }

    const editor::CommandResult nullExecute = dispatcher.execute(std::unique_ptr<editor::ICommand>{});
    if (nullExecute || nullExecute.message.empty()) {
        return TestResult::fail("execute(unique_ptr) should reject null commands");
    }

    auto ownedCommand = std::make_unique<CountingCommand>();
    const editor::CommandResult ownedExecute = dispatcher.execute(std::move(ownedCommand));
    if (!ownedExecute || ownedExecute.message != "executed") {
        return TestResult::fail("execute(unique_ptr) should execute owned commands");
    }

    return TestResult::pass();
}

} // namespace locus::tests
