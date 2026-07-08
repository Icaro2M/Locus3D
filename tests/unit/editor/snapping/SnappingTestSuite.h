/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file SnappingTestSuite.h
 * @brief Declarations for editor snapping unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_snap_settings_and_solver_tests();
[[nodiscard]] TestResult run_numeric_snap_provider_tests();
[[nodiscard]] TestResult run_scene_snap_provider_tests();

} // namespace locus::tests
