/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file MeshCommandTestSuite.h
 * @brief Declarations for mesh editor command unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_mesh_snapshot_tests();
[[nodiscard]] TestResult run_replace_mesh_command_tests();
[[nodiscard]] TestResult run_apply_mesh_operation_command_tests();
[[nodiscard]] TestResult run_edit_mesh_selection_command_tests();

} // namespace locus::tests
