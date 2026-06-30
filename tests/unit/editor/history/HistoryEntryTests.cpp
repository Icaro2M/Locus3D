/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "HistoryTestSuite.h"

#include "editor/history/HistoryEntry.h"

namespace {

class NamedHistoryCommand final : public locus::editor::ICommand {
public:
    explicit NamedHistoryCommand(std::string_view commandName)
        : commandName_(commandName)
    {
    }

    [[nodiscard]] std::string_view name() const override
    {
        return commandName_;
    }

    locus::editor::CommandResult execute(locus::editor::CommandContext&) override
    {
        return locus::editor::CommandResult::ok();
    }

    locus::editor::CommandResult undo(locus::editor::CommandContext&) override
    {
        return locus::editor::CommandResult::ok();
    }

private:
    std::string_view commandName_;
};

} // namespace

namespace locus::tests {

TestResult run_history_entry_tests()
{
    editor::HistoryEntry empty;
    if (empty.is_valid() || !empty.command_name().empty()) {
        return TestResult::fail("default HistoryEntry should be invalid and unnamed");
    }

    auto command = std::make_unique<NamedHistoryCommand>("Create Node");
    editor::HistoryEntry entry(
        std::move(command),
        editor::CommandResult::ok(editor::EditorDirtyFlags::Scene, "created"));

    if (!entry.is_valid() ||
        entry.command_name() != "Create Node" ||
        entry.command().name() != "Create Node" ||
        entry.last_result().message != "created") {
        return TestResult::fail("HistoryEntry should own command and capture command name/result");
    }

    entry.set_last_result(editor::CommandResult::ok(
        editor::EditorDirtyFlags::Selection,
        "updated"));

    if (entry.last_result().message != "updated" ||
        entry.last_result().dirtyFlags != editor::EditorDirtyFlags::Selection) {
        return TestResult::fail("set_last_result should replace stored metadata");
    }

    std::unique_ptr<editor::ICommand> released = entry.take_command();
    if (!released || released->name() != "Create Node" || entry.is_valid()) {
        return TestResult::fail("take_command should release ownership and invalidate entry");
    }

    if (entry.command_name() != "Create Node") {
        return TestResult::fail("take_command should preserve captured command name metadata");
    }

    return TestResult::pass();
}

} // namespace locus::tests
