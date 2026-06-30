/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file CommandTestSuite.h
 * @brief Declarations for editor command unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_command_result_tests();
[[nodiscard]] TestResult run_command_context_tests();
[[nodiscard]] TestResult run_command_registry_tests();
[[nodiscard]] TestResult run_command_dispatcher_tests();

} // namespace locus::tests
