/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file SelectionTestSuite.h
 * @brief Declarations for editor selection unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_selection_set_tests();
[[nodiscard]] TestResult run_object_selection_tests();
[[nodiscard]] TestResult run_mesh_selection_tests();
[[nodiscard]] TestResult run_selection_state_tests();
[[nodiscard]] TestResult run_selection_controller_tests();
[[nodiscard]] TestResult run_selection_shape_types_tests();
[[nodiscard]] TestResult run_select_tool_visual_state_tests();

} // namespace locus::tests
