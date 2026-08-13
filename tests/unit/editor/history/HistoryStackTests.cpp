/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "HistoryTestSuite.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"

namespace {

class HistoryCommand final : public locus::editor::ICommand {
public:
    HistoryCommand(
        std::string_view commandName,
        bool undoable = true,
        bool executeSuccess = true,
        bool undoSuccess = true,
        bool redoSuccess = true)
        : commandName_(commandName)
        , undoable_(undoable)
        , executeSuccess_(executeSuccess)
        , undoSuccess_(undoSuccess)
        , redoSuccess_(redoSuccess)
    {
    }

    [[nodiscard]] std::string_view name() const override
    {
        return commandName_;
    }

    [[nodiscard]] bool is_undoable() const override
    {
        return undoable_;
    }

    locus::editor::CommandResult execute(locus::editor::CommandContext&) override
    {
        ++executeCount;
        if (!executeSuccess_) {
            return locus::editor::CommandResult::fail("execute failed");
        }

        return locus::editor::CommandResult::ok(
            locus::editor::EditorDirtyFlags::Scene,
            "executed");
    }

    locus::editor::CommandResult undo(locus::editor::CommandContext&) override
    {
        ++undoCount;
        if (!undoSuccess_) {
            return locus::editor::CommandResult::fail("undo failed");
        }

        return locus::editor::CommandResult::ok(
            locus::editor::EditorDirtyFlags::Selection,
            "undone");
    }

    locus::editor::CommandResult redo(locus::editor::CommandContext&) override
    {
        ++redoCount;
        if (!redoSuccess_) {
            return locus::editor::CommandResult::fail("redo failed");
        }

        return locus::editor::CommandResult::ok(
            locus::editor::EditorDirtyFlags::Mesh,
            "redone");
    }

    int executeCount = 0;
    int undoCount = 0;
    int redoCount = 0;

private:
    std::string_view commandName_;
    bool undoable_ = true;
    bool executeSuccess_ = true;
    bool undoSuccess_ = true;
    bool redoSuccess_ = true;
};

} // namespace

namespace locus::tests {

TestResult run_history_stack_tests()
{
    editor::Editor editor;
    editor::CommandDispatcher dispatcher(editor);
    editor::HistoryStack history;

    if (!history.empty() ||
        history.can_undo() ||
        history.can_redo() ||
        !history.undo_name().empty() ||
        !history.redo_name().empty()) {
        return TestResult::fail("new HistoryStack should start empty");
    }

    if (history.execute(dispatcher, nullptr) ||
        !history.empty()) {
        return TestResult::fail("execute should reject null commands without storing history");
    }

    auto create = std::make_unique<HistoryCommand>("Create");
    const editor::CommandResult createResult = history.execute(dispatcher, std::move(create));
    if (!createResult ||
        !history.can_undo() ||
        history.can_redo() ||
        history.undo_size() != 1 ||
        history.undo_name() != "Create") {
        return TestResult::fail("successful undoable execute should push undo history");
    }

    auto transient = std::make_unique<HistoryCommand>("Transient", false);
    if (!history.execute(dispatcher, std::move(transient)) ||
        history.undo_size() != 1 ||
        history.undo_name() != "Create") {
        return TestResult::fail("successful non-undoable commands should execute without storing history");
    }

    auto failing = std::make_unique<HistoryCommand>("Failing", true, false);
    if (history.execute(dispatcher, std::move(failing)) ||
        history.undo_size() != 1) {
        return TestResult::fail("failed execute should not push undo history");
    }

    const editor::CommandResult undoResult = history.undo(dispatcher);
    if (!undoResult ||
        history.can_undo() ||
        !history.can_redo() ||
        history.redo_size() != 1 ||
        history.redo_name() != "Create") {
        return TestResult::fail("undo should move latest undo entry to redo stack");
    }

    const editor::CommandResult redoResult = history.redo(dispatcher);
    if (!redoResult ||
        !history.can_undo() ||
        history.can_redo() ||
        history.undo_name() != "Create") {
        return TestResult::fail("redo should move latest redo entry back to undo stack");
    }

    auto second = std::make_unique<HistoryCommand>("Second");
    if (!history.execute(dispatcher, std::move(second)) ||
        history.undo_size() != 2 ||
        history.undo_name() != "Second") {
        return TestResult::fail("executing another command should append to undo stack");
    }

    history.undo(dispatcher);
    if (!history.can_redo()) {
        return TestResult::fail("undo should create redo availability before redo clearing checks");
    }

    auto third = std::make_unique<HistoryCommand>("Third");
    history.execute(dispatcher, std::move(third));
    if (history.can_redo()) {
        return TestResult::fail("new successful execution should clear redo stack");
    }

    history.set_max_entries(2);
    if (history.max_entries() != 2 || history.undo_size() != 2) {
        return TestResult::fail("set_max_entries should trim undo stack to the configured limit");
    }

    auto fourth = std::make_unique<HistoryCommand>("Fourth");
    history.execute(dispatcher, std::move(fourth));
    if (history.undo_size() != 2 || history.undo_name() != "Fourth") {
        return TestResult::fail("history should retain only the newest entries when limited");
    }

    history.clear_redo();
    if (history.can_redo()) {
        return TestResult::fail("clear_redo should remove redo entries only");
    }

    history.clear();
    if (!history.empty() || history.can_undo() || history.can_redo()) {
        return TestResult::fail("clear should remove undo and redo history");
    }

    if (history.undo(dispatcher) || history.redo(dispatcher)) {
        return TestResult::fail("undo/redo should fail when no matching history is available");
    }

    auto pushed = std::make_unique<HistoryCommand>("Pushed");
    if (!history.push_executed(std::move(pushed), editor::CommandResult::ok()) ||
        history.undo_name() != "Pushed") {
        return TestResult::fail("push_executed should store already executed undoable commands");
    }

    auto notUndoable = std::make_unique<HistoryCommand>("Not Undoable", false);
    if (history.push_executed(std::move(notUndoable), editor::CommandResult::ok()) ||
        history.push_executed(nullptr, editor::CommandResult::ok()) ||
        history.push_executed(
            std::make_unique<HistoryCommand>("Failed Result"),
            editor::CommandResult::fail("failed"))) {
        return TestResult::fail("push_executed should reject invalid command/result combinations");
    }

    history.clear();
    history.mark_clean();
    if (!history.is_clean()) {
        return TestResult::fail("mark_clean should mark the current history state clean");
    }

    history.execute(
        dispatcher,
        std::make_unique<HistoryCommand>("Move"));
    if (history.is_clean()) {
        return TestResult::fail("executing an undoable command should leave the clean checkpoint");
    }

    history.undo(dispatcher);
    if (!history.is_clean()) {
        return TestResult::fail("undo should return clean when it reaches the saved state");
    }

    history.redo(dispatcher);
    if (history.is_clean()) {
        return TestResult::fail("redo should leave the saved state again");
    }

    history.undo(dispatcher);
    history.execute(
        dispatcher,
        std::make_unique<HistoryCommand>("Rename"));
    if (history.is_clean()) {
        return TestResult::fail("branching after undo should not reuse the clean state id");
    }

    return TestResult::pass();
}

} // namespace locus::tests
