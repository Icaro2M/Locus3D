/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SelectionTestSuite.h"

#include "editor/Editor.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/selection/SelectTool.h"

namespace locus::tests {

namespace {

    [[nodiscard]] editor::ToolEvent pointer_event(
        editor::ToolEventType type,
        const glm::vec2& position)
    {
        editor::ToolEvent event{};
        event.type = type;
        event.button = editor::ToolPointerButton::Primary;
        event.pointer.viewportPosition = position;
        event.pointer.viewportSize = glm::vec2{ 200.0f, 120.0f };
        return event;
    }

    [[nodiscard]] TestResult activate_tool(
        editor::SelectTool& tool,
        editor::ToolContext& context)
    {
        const editor::ToolResult result = tool.activate(context);
        if (result.failed()) {
            return TestResult::fail("SelectTool should activate for visual state tests");
        }

        return TestResult::pass();
    }

    [[nodiscard]] TestResult begin_drag(
        editor::SelectTool& tool,
        editor::ToolContext& context,
        const glm::vec2& start)
    {
        const editor::ToolResult result =
            tool.handle_event(
                context,
                pointer_event(editor::ToolEventType::PointerPress, start));
        if (result.failed()) {
            return TestResult::fail("SelectTool should accept primary press");
        }

        return TestResult::pass();
    }

    [[nodiscard]] TestResult move_drag(
        editor::SelectTool& tool,
        editor::ToolContext& context,
        const glm::vec2& current)
    {
        const editor::ToolResult result =
            tool.handle_event(
                context,
                pointer_event(editor::ToolEventType::PointerMove, current));
        if (result.failed()) {
            return TestResult::fail("SelectTool should accept pointer move");
        }

        return TestResult::pass();
    }

} // namespace

TestResult run_select_tool_visual_state_tests()
{
    editor::Editor editor{};
    editor::ToolContext context{ editor };
    editor::SelectTool tool{};

    TestResult result = activate_tool(tool, context);
    if (!result.success) {
        return result;
    }

    if (tool.point_selection_depth_mode() !=
            editor::SelectionDepthMode::VisibleOnly ||
        tool.region_selection_depth_mode() !=
            editor::SelectionDepthMode::VisibleOnly) {
        return TestResult::fail(
            "SelectTool should default selection depth to visible-only");
    }

    tool.set_selection_depth_modes(
        editor::SelectionDepthMode::Through,
        editor::SelectionDepthMode::Through);

    if (tool.point_selection_depth_mode() !=
            editor::SelectionDepthMode::Through ||
        tool.region_selection_depth_mode() !=
            editor::SelectionDepthMode::Through) {
        return TestResult::fail(
            "SelectTool should expose viewport selection depth policy");
    }

    if (tool.selection_region_visual_state().visible) {
        return TestResult::fail(
            "Selection region visual state should be hidden initially");
    }

    result = begin_drag(tool, context, { 10.0f, 20.0f });
    if (!result.success) {
        return result;
    }

    if (tool.selection_region_visual_state().visible) {
        return TestResult::fail(
            "Selection region should stay hidden before drag threshold");
    }

    result = move_drag(tool, context, { 12.0f, 22.0f });
    if (!result.success) {
        return result;
    }

    if (tool.selection_region_visual_state().visible) {
        return TestResult::fail(
            "Selection region should stay hidden below drag threshold");
    }

    result = move_drag(tool, context, { 30.0f, 45.0f });
    if (!result.success) {
        return result;
    }

    editor::SelectionRegionVisualState visualState =
        tool.selection_region_visual_state();
    if (!visualState.visible ||
        visualState.start != glm::vec2{ 10.0f, 20.0f } ||
        visualState.current != glm::vec2{ 30.0f, 45.0f }) {
        return TestResult::fail(
            "Selection region should expose start and current after threshold");
    }

    result = move_drag(tool, context, { 55.0f, 65.0f });
    if (!result.success) {
        return result;
    }

    visualState = tool.selection_region_visual_state();
    if (!visualState.visible ||
        visualState.current != glm::vec2{ 55.0f, 65.0f }) {
        return TestResult::fail(
            "Selection region should update the current drag point");
    }

    (void)tool.handle_event(
        context,
        pointer_event(editor::ToolEventType::PointerRelease, { 55.0f, 65.0f }));

    if (tool.selection_region_visual_state().visible) {
        return TestResult::fail(
            "Selection region should be hidden after pointer release");
    }

    result = begin_drag(tool, context, { 20.0f, 20.0f });
    if (!result.success) {
        return result;
    }

    result = move_drag(tool, context, { 40.0f, 40.0f });
    if (!result.success) {
        return result;
    }

    (void)tool.handle_event(
        context,
        editor::ToolEvent{ editor::ToolEventType::Cancel });

    if (tool.selection_region_visual_state().visible) {
        return TestResult::fail(
            "Selection region should be hidden after cancellation");
    }

    result = begin_drag(tool, context, { 15.0f, 15.0f });
    if (!result.success) {
        return result;
    }

    result = move_drag(tool, context, { 35.0f, 35.0f });
    if (!result.success) {
        return result;
    }

    (void)tool.deactivate(context);

    if (tool.selection_region_visual_state().visible) {
        return TestResult::fail(
            "Selection region should be hidden after tool deactivation");
    }

    return TestResult::pass();
}

} // namespace locus::tests
