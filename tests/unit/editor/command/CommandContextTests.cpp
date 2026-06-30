/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CommandTestSuite.h"

#include "editor/command/CommandContext.h"

namespace locus::tests {

TestResult run_command_context_tests()
{
    editor::Editor editor;
    editor.clear_dirty();

    editor::CommandContext context(editor);

    if (&context.editor() != &editor || &context.state() != &editor.state()) {
        return TestResult::fail("CommandContext should expose the wrapped editor and state");
    }

    context.mark_dirty(editor::EditorDirtyFlags::Render);
    if (!editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Render)) {
        return TestResult::fail("CommandContext::mark_dirty should mark the wrapped editor");
    }

    editor.clear_dirty();
    const editor::SceneNodeId node = context.scene().create_empty("Command node");
    if (!node.is_valid() ||
        !editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Scene)) {
        return TestResult::fail("mutable scene access through CommandContext should use Editor facade");
    }

    editor.clear_dirty();
    context.selection().objects().set(node);
    if (!context.selection().objects().contains(node) ||
        !editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Selection)) {
        return TestResult::fail("mutable selection access through CommandContext should mark selection dirty");
    }

    if (!context.selection_controller().select_object(node)) {
        return TestResult::fail("CommandContext should expose the selection controller");
    }

    return TestResult::pass();
}

} // namespace locus::tests
