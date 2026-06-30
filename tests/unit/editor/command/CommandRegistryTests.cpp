/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CommandTestSuite.h"

#include "editor/command/CommandRegistry.h"

namespace {

class NamedCommand final : public locus::editor::ICommand {
public:
    explicit NamedCommand(std::string_view name)
        : name_(name)
    {
    }

    [[nodiscard]] std::string_view name() const override
    {
        return name_;
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
    std::string_view name_;
};

} // namespace

namespace locus::tests {

TestResult run_command_registry_tests()
{
    editor::CommandRegistry registry;

    if (!registry.empty() || registry.size() != 0) {
        return TestResult::fail("new CommandRegistry should start empty");
    }

    if (registry.register_command("", [] { return std::make_unique<NamedCommand>("invalid"); }) ||
        registry.register_command("null", {})) {
        return TestResult::fail("register_command should reject empty ids and empty factories");
    }

    if (!registry.register_command("scene.create_empty", [] {
            return std::make_unique<NamedCommand>("Create Empty");
        })) {
        return TestResult::fail("register_command should insert a valid factory");
    }

    if (registry.register_command("scene.create_empty", [] {
            return std::make_unique<NamedCommand>("Duplicate");
        })) {
        return TestResult::fail("register_command should reject duplicate ids");
    }

    if (!registry.contains("scene.create_empty") || registry.size() != 1) {
        return TestResult::fail("contains and size should reflect registered factories");
    }

    std::unique_ptr<editor::ICommand> command = registry.create("scene.create_empty");
    if (!command || command->name() != "Create Empty") {
        return TestResult::fail("create should instantiate commands from registered factories");
    }

    if (!registry.replace_command("scene.create_empty", [] {
            return std::make_unique<NamedCommand>("Create Empty Replacement");
        })) {
        return TestResult::fail("replace_command should replace existing valid factories");
    }

    command = registry.create("scene.create_empty");
    if (!command || command->name() != "Create Empty Replacement") {
        return TestResult::fail("replace_command should update future command creation");
    }

    if (!registry.replace_command("selection.clear", [] {
            return std::make_unique<NamedCommand>("Clear Selection");
        }) ||
        registry.command_ids().size() != 2) {
        return TestResult::fail("replace_command should also insert new valid ids");
    }

    if (!registry.unregister_command("selection.clear") ||
        registry.unregister_command("selection.clear") ||
        registry.contains("selection.clear")) {
        return TestResult::fail("unregister_command should remove existing ids only once");
    }

    registry.clear();
    if (!registry.empty() || registry.create("scene.create_empty")) {
        return TestResult::fail("clear should remove every registered command factory");
    }

    return TestResult::pass();
}

} // namespace locus::tests
