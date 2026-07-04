/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "HistoryTestSuite.h"

#include "editor/history/HistoryConfig.h"

namespace locus::tests {

TestResult run_history_config_tests()
{
    if (editor::UnlimitedHistoryEntries != 0u ||
        editor::DefaultHistoryMaxEntries != 128u ||
        editor::MinHistoryMaxEntries != 1u ||
        editor::MaxHistoryMaxEntries != 4096u) {
        return TestResult::fail("history configuration constants should expose the expected public values");
    }

    const editor::HistoryConfig defaults;
    if (defaults.maxEntries != editor::DefaultHistoryMaxEntries ||
        !defaults.enabled) {
        return TestResult::fail("HistoryConfig should default to bounded enabled history");
    }

    if (editor::normalize_history_max_entries(editor::UnlimitedHistoryEntries) !=
            editor::UnlimitedHistoryEntries ||
        editor::normalize_history_max_entries(editor::MinHistoryMaxEntries) !=
            editor::MinHistoryMaxEntries ||
        editor::normalize_history_max_entries(editor::DefaultHistoryMaxEntries) !=
            editor::DefaultHistoryMaxEntries ||
        editor::normalize_history_max_entries(editor::MaxHistoryMaxEntries) !=
            editor::MaxHistoryMaxEntries ||
        editor::normalize_history_max_entries(editor::MaxHistoryMaxEntries + 1u) !=
            editor::MaxHistoryMaxEntries) {
        return TestResult::fail("normalize_history_max_entries should preserve unlimited and clamp bounded values");
    }

    editor::HistoryConfig lowConfig;
    lowConfig.maxEntries = editor::MaxHistoryMaxEntries + 1u;
    lowConfig.enabled = false;

    const editor::HistoryConfig normalizedHigh = editor::normalize_history_config(lowConfig);
    if (normalizedHigh.maxEntries != editor::MaxHistoryMaxEntries ||
        normalizedHigh.enabled) {
        return TestResult::fail("normalize_history_config should clamp high values and preserve enabled state");
    }

    editor::HistoryConfig unlimitedConfig;
    unlimitedConfig.maxEntries = editor::UnlimitedHistoryEntries;
    unlimitedConfig.enabled = true;

    const editor::HistoryConfig normalizedUnlimited =
        editor::normalize_history_config(unlimitedConfig);
    if (normalizedUnlimited.maxEntries != editor::UnlimitedHistoryEntries ||
        !normalizedUnlimited.enabled) {
        return TestResult::fail("normalize_history_config should preserve unlimited history");
    }

    return TestResult::pass();
}

} // namespace locus::tests
