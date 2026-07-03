/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file DocumentCommandTestSuite.h
 * @brief Declarations for document editor command unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_clear_scene_command_tests();
[[nodiscard]] TestResult run_import_mesh_command_tests();

} // namespace locus::tests
