/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file CommandCommandsTestSuite.h
 * @brief Declarations for concrete editor command unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_create_node_command_tests();
[[nodiscard]] TestResult run_node_metadata_command_tests();
[[nodiscard]] TestResult run_node_hierarchy_command_tests();
[[nodiscard]] TestResult run_node_transform_command_tests();
[[nodiscard]] TestResult run_object_selection_command_tests();
[[nodiscard]] TestResult run_selection_mode_command_tests();
[[nodiscard]] TestResult run_mesh_selection_command_tests();

} // namespace locus::tests
