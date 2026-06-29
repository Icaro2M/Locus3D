/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SelectionTestSuite.h"

#include "editor/selection/SelectionSet.h"

namespace locus::tests {

TestResult run_selection_set_tests()
{
    editor::SelectionSet<int> selection;

    if (!selection.empty() || selection.size() != 0) {
        return TestResult::fail("new SelectionSet should start empty");
    }

    if (!selection.add(2) || !selection.add(4) || selection.add(2)) {
        return TestResult::fail("add should keep ordered unique items");
    }

    if (selection.items().size() != 2 ||
        selection.items()[0] != 2 ||
        selection.items()[1] != 4) {
        return TestResult::fail("SelectionSet should preserve insertion order");
    }

    if (!selection.toggle(6) || !selection.contains(6)) {
        return TestResult::fail("toggle should add missing items");
    }

    if (selection.toggle(6) || selection.contains(6)) {
        return TestResult::fail("toggle should remove existing items");
    }

    selection.set({ 4, 2, 4, 8 });
    if (selection.size() != 3 ||
        selection.items()[0] != 4 ||
        selection.items()[1] != 2 ||
        selection.items()[2] != 8) {
        return TestResult::fail("set(vector) should remove duplicates and preserve first occurrence");
    }

    if (!selection.remove(2) || selection.remove(99) || selection.contains(2)) {
        return TestResult::fail("remove should only succeed for selected items");
    }

    selection.clear();
    if (!selection.empty()) {
        return TestResult::fail("clear should remove every selected item");
    }

    return TestResult::pass();
}

} // namespace locus::tests
