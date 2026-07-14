/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file PrimitivesTestSuite.h
 * @brief Declarations for graphics primitives unit test groups.
 */

#include "common/TestResult.h"

namespace locus::tests {

[[nodiscard]] TestResult run_primitive_mesh_tests();
[[nodiscard]] TestResult run_primitive_builder_tests();
[[nodiscard]] TestResult run_primitive_mesh_converter_tests();

} // namespace locus::tests
