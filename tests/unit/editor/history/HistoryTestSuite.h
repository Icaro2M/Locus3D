/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file HistoryTestSuite.h
 * @brief Declarations for editor history unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_history_entry_tests();
[[nodiscard]] TestResult run_history_stack_tests();

} // namespace locus::tests
