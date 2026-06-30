/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CommandTestSuite.h"

#include "editor/command/CommandResult.h"

namespace locus::tests {

TestResult run_command_result_tests()
{
    const editor::CommandResult ok = editor::CommandResult::ok(
        editor::EditorDirtyFlags::Scene,
        "created");

    if (!ok || !ok.success ||
        ok.dirtyFlags != editor::EditorDirtyFlags::Scene ||
        ok.message != "created") {
        return TestResult::fail("CommandResult::ok should preserve success, flags, and message");
    }

    const editor::CommandResult fail = editor::CommandResult::fail(
        "invalid command",
        editor::EditorDirtyFlags::Selection);

    if (fail || fail.success ||
        fail.dirtyFlags != editor::EditorDirtyFlags::Selection ||
        fail.message != "invalid command") {
        return TestResult::fail("CommandResult::fail should preserve failure, flags, and message");
    }

    return TestResult::pass();
}

} // namespace locus::tests
