/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SelectionTestSuite.h"

#include "editor/tools/selection/shapes/SelectionShapeTypes.h"

namespace locus::tests {

TestResult run_selection_shape_types_tests()
{
    const editor::ScreenSelectionRect rect =
        editor::ScreenSelectionRect::from_points(
            { 40.0f, 80.0f },
            { 10.0f, 20.0f });

    if (rect.min.x != 10.0f ||
        rect.min.y != 20.0f ||
        rect.max.x != 40.0f ||
        rect.max.y != 80.0f) {
        return TestResult::fail(
            "ScreenSelectionRect should normalize inverted drag points");
    }

    const editor::ScreenSelectionRect clipped =
        editor::ScreenSelectionRect::from_points(
            { -5.0f, -6.0f },
            { 120.0f, 90.0f })
        .clipped({ 100.0f, 50.0f });

    if (clipped.min.x != 0.0f ||
        clipped.min.y != 0.0f ||
        clipped.max.x != 99.0f ||
        clipped.max.y != 49.0f) {
        return TestResult::fail(
            "ScreenSelectionRect should clip to viewport pixel bounds");
    }

    if (!rect.contains({ 25.0f, 40.0f }) ||
        rect.contains({ 5.0f, 40.0f })) {
        return TestResult::fail(
            "ScreenSelectionRect should test point containment");
    }

    if (!rect.intersects_segment({ 0.0f, 50.0f }, { 100.0f, 50.0f }) ||
        rect.intersects_segment({ 0.0f, 0.0f }, { 5.0f, 5.0f })) {
        return TestResult::fail(
            "ScreenSelectionRect should test segment intersection");
    }

    if (editor::selection_operation_from_modifiers(
            editor::ToolModifiers::None)
        != editor::SelectionOperation::Replace ||
        editor::selection_operation_from_modifiers(
            editor::ToolModifiers::Additive)
        != editor::SelectionOperation::Add ||
        editor::selection_operation_from_modifiers(
            editor::ToolModifiers::Toggle)
        != editor::SelectionOperation::Toggle ||
        editor::selection_operation_from_modifiers(
            editor::ToolModifiers::Additive |
            editor::ToolModifiers::Toggle)
        != editor::SelectionOperation::Subtract) {
        return TestResult::fail(
            "Selection modifiers should map to stable operations");
    }

    return TestResult::pass();
}

} // namespace locus::tests
