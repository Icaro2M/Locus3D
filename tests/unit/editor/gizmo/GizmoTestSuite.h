/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file GizmoTestSuite.h
 * @brief Declarations for editor gizmo unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_gizmo_state_tests();
[[nodiscard]] TestResult run_gizmo_constraint_tests();
[[nodiscard]] TestResult run_gizmo_snap_tests();
[[nodiscard]] TestResult run_transform_gizmo_tests();
[[nodiscard]] TestResult run_gizmo_controller_tests();

} // namespace locus::tests
