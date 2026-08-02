/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file ActionTestSuite.h
 * @brief Declarations for editor action unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_bridge_edge_action_tests();
[[nodiscard]] TestResult run_fill_hole_action_tests();
[[nodiscard]] TestResult run_flip_face_action_tests();
[[nodiscard]] TestResult run_delete_action_tests();
[[nodiscard]] TestResult run_dissolve_action_tests();

} // namespace locus::tests
