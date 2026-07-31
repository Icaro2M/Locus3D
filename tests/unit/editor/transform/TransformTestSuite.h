/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file TransformTestSuite.h
 * @brief Declarations for editor transform unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_transform_target_tests();
[[nodiscard]] TestResult run_transform_pivot_resolver_tests();
[[nodiscard]] TestResult run_transform_session_tests();
[[nodiscard]] TestResult run_mesh_transform_target_resolver_tests();
[[nodiscard]] TestResult run_mesh_transform_tool_session_tests();
[[nodiscard]] TestResult run_transform_tool_selection_tests();

} // namespace locus::tests
