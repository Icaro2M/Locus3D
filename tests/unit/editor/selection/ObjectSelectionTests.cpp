/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SelectionTestSuite.h"

#include "editor/selection/ObjectSelection.h"

namespace locus::tests {

TestResult run_object_selection_tests()
{
    const editor::SceneNodeId a{ 1 };
    const editor::SceneNodeId b{ 2 };
    const editor::SceneNodeId c{ 3 };
    const editor::SceneNodeId d{ 5 };

    editor::ObjectSelection selection;

    selection.set(a);
    if (!selection.contains(a) || selection.active() != a || selection.size() != 1) {
        return TestResult::fail("set should select one object and make it active");
    }

    selection.set({ a, b, a, c });
    if (selection.size() != 3 || selection.active() != c) {
        return TestResult::fail("set(vector) should dedupe and use last selected as active");
    }

    selection.set({ a, b, c }, b);
    if (selection.active() != b) {
        return TestResult::fail("set(vector, active) should keep a selected active object");
    }

    if (selection.add(b) || !selection.add(editor::SceneNodeId{ 4 })) {
        return TestResult::fail("add should reject duplicates and accept new valid ids");
    }

    if (!selection.toggle(d) || selection.toggle(d)) {
        return TestResult::fail("toggle should add then remove object ids");
    }

    selection.set_hovered(b);
    if (!selection.remove(b) || selection.active() == b || selection.hovered().is_valid()) {
        return TestResult::fail("remove should update active and hovered object state");
    }

    selection.set_active(c);
    if (!selection.contains(c) || selection.active() != c) {
        return TestResult::fail("set_active should add valid objects that are not selected");
    }

    selection.clear();
    if (!selection.empty() || selection.active().is_valid() || selection.hovered().is_valid()) {
        return TestResult::fail("clear should reset selected, active, and hovered objects");
    }

    return TestResult::pass();
}

} // namespace locus::tests
